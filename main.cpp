#include <iostream>
#include <queue>
#include <chrono>
#include <thread>

using namespace std;
using namespace std::chrono_literals;

int taille_file = 4;
std::queue<int> file;


void barbier(){
    while (file.size() != 0){ // si la file d'attente n'est pas vide
        int id_client = file.front(); // récupère l'id du client le plus ancien de la file d'attente
        file.pop(); // supprime le client le plus ancien de la liste

        cout << "Le barbier coiffe le client " << id_client << endl;
        std::this_thread::sleep_for(2s); // Simule la durée de la coupe
        cout << "Le client " << id_client << " a fini et quitte le salon." << endl;
    }
}

void client(int id){
    if (file.size() <= taille_file){ // si la file d'attente n'est pas pleine
        file.push(id); // ajoute le client à la fin de la file d'attente
        cout << "Le client " << id << " s'assoit dans la salle d'attente." << endl;
    }
    else{
        cout << "Le client " << id << " repart. Car la salle est pleine" << endl;
    }

    cout << "Le client " << id << " est en train de se faire coiffer" << endl;
}

int main()
{



    cout << "Hello World!" << endl;
    return 0;
}
