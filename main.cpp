#include <iostream>
#include <queue>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <semaphore>

using namespace std;
using namespace std::chrono_literals;

int taille_file = 4;
std::queue<int> file;

std::mutex mut;
std::condition_variable cv_barbier;
std::condition_variable cv_client;
int client_appele = -1;
bool salon_ferme = false;

std::binary_semaphore coupe_finie(0); // Jeton pour que le barbier indique au client que sa coupe est fini
std::binary_semaphore client_parti(0); // Jeton pour que le client indique au barbier qu'il a quitté le fauteuil

void barbier(){
    while (!salon_ferme){
        std::unique_lock<std::mutex> verrou(mut); // On verrouille le barbier

        // Tant que la file est vide et que le salon n'est pas fermé, le barbier dort
        while (file.size() == 0 && !salon_ferme) {
            cv_barbier.wait(verrou);
        }

        // Si le salon n'est pas fermé, le barbier s'occupe du premier client de la file
        if (!salon_ferme) {
            int id_client = file.front();
            file.pop();
            client_appele = id_client;

            verrou.unlock(); // On déverrouille manuellement avant de coiffer

            cv_client.notify_all();

            cout << "Le barbier coiffe le client " << id_client << endl;
            std::this_thread::sleep_for(2s);
            cout << "Le client " << id_client << " a fini et quitte le salon." << endl;

            coupe_finie.release();
            client_parti.acquire();
        }
    }
}

void client(int id){
    std::unique_lock<std::mutex> verrou(mut);
    if (file.size() < taille_file){ // si la file d'attente n'est pas pleine
        file.push(id); // ajoute le client à la fin de la file d'attente
        cout << "Le client " << id << " s'assoit dans la salle d'attente." << endl;
        cv_barbier.notify_one(); // réveille le barbier s'il dort

        // Tant que le numéro appelé n'est pas le mien, je me rendors
        while (client_appele != id) {
            cv_client.wait(verrou);
        }
        verrou.unlock();

        coupe_finie.acquire();   // attend la fin de la coupe
        client_parti.release();  // libère le fauteuil
    }
    else{
        cout << "Le client " << id << " repart car la salle est pleine" << endl;
    }
}

int main()
{
    std::thread t_barbier(barbier);
    std::vector<std::thread> clients;

    for (int i = 0; i < 20; ++i){
        clients.emplace_back(client, i);
        std::this_thread::sleep_for(1s); // un client arrive toutes les 1 s
    }

    // On attend que tous les clients soient passés ou repartis
    for (std::thread & t : clients)
        t.join();

    // Plus aucun client : on ferme le salon et on réveille le barbier pour qu'il s'arrête
    mut.lock();
    salon_ferme = true;
    mut.unlock();
    cv_barbier.notify_one();

    t_barbier.join();
    return 0;
}