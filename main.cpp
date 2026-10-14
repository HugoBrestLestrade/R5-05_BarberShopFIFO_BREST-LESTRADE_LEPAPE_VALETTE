#include <iostream>
#include <queue>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <semaphore>
#include <atomic>

using namespace std;

const size_t capacite_file = 4; // Nombre max d'étudiants dans la file d'attente
std::queue<int> file_attente; // File d'attente devant la caisse

std::mutex tablette; // Tablette du CROUS, représente l'accès exclusif à la file d'attente
std::condition_variable reveil_employe; // Pour sortir l'employé de TikTok
std::condition_variable reveil_etudiants; // Pour sortir les étudiants de TikTok
int etudiant_appele = -1; // Numéro de l'étudiant appelé par l'employé != étudiant au hasard qui serait le problème classique du barbershop
std::atomic<bool> cafeteria_fermee(false);

std::binary_semaphore commande_servie(0); // Jeton pour que l'employé indique à l'étudiant que sa commande est prête
std::binary_semaphore place_paiement_liberee(0); // Jeton pour que l'étudiant indique qu'il a quitté la place de paiement

void employe_crous(){
    while (!cafeteria_fermee){
        std::unique_lock<std::mutex> verrou(tablette); // L'employé consulte la tablette

        // Tant que la file est vide et que la cafétéria est ouverte, l'employé scrolle sur TikTok
        if (file_attente.empty() && !cafeteria_fermee)
            cout << "L'employé du CROUS n'a personne : il scrolle sur TikTok." << endl;
        reveil_employe.wait(verrou, [&] { return !file_attente.empty() || cafeteria_fermee; });

        // Si la cafétéria est ouverte, l'employé appelle le premier étudiant de la file
        if (!cafeteria_fermee) {
            int id_etudiant = file_attente.front(); // Récupère l'identifiant du premier étudiant
            file_attente.pop(); // Puis le fait sortir de la file d'attente (l'étudiant scrolle encore)
            etudiant_appele = id_etudiant;

            verrou.unlock(); // L'employé repose la tablette avant de servir

            reveil_etudiants.notify_all(); // Réveille les étudiants pour que le bon se manifeste

            cout << "L'employé prend la commande de l'étudiant " << id_etudiant << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 2s = le temps que l'employé serve l'étudiant
            commande_servie.release(); // Remet la commande à l'étudiant
            place_paiement_liberee.acquire(); // Attend que la place de paiement soit libre
            cout << "L'étudiant " << id_etudiant << " récupère sa commande et quitte la cafétéria." << endl;
        }
    }
}

void etudiant(int id){
    std::unique_lock<std::mutex> verrou(tablette); // L'étudiant prend la tablette
    if (file_attente.size() < capacite_file){ // Vérifie que la file n'est pas pleine
        file_attente.push(id); // Signale sa présence sur la tablette
        cout << "L'étudiant " << id << " entre, se signale sur la tablette et scrolle sur TikTok." << endl;
        reveil_employe.notify_one(); // Sort l'employé de TikTok s'il y est

        // Tant que ce n'est pas le numéro de l'étudiant, il continue de scroller
        reveil_etudiants.wait(verrou, [&] { return etudiant_appele == id; });
        
        verrou.unlock();
        commande_servie.acquire(); // Attend sa commande à la place de paiement
        place_paiement_liberee.release(); // Libère la place de paiement
    }
    else{
        cout << "L'étudiant " << id << " voit à travers la vitre que la file est pleine et repart." << endl;
    }
}

int main()
{
    std::thread thread_employe(employe_crous);
    std::vector<std::thread> threads_etudiants;

    for (int i = 0; i < 20; ++i){
        threads_etudiants.emplace_back(etudiant, i);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Un étudiant arrive toutes les secondes
    }

    // On attend que tous les étudiants soient servis ou repartis
    for (std::thread & t : threads_etudiants)
        t.join();

    // On ferme la cafétéria car il n'y a plus aucun étudiant
    {
        std::lock_guard<std::mutex> verrou(tablette);
        cafeteria_fermee = true;
    }
     // On sort l'employé de TikTok une dernière fois pour lui dire que la cafétéria est fermée
    reveil_employe.notify_one();
    thread_employe.join(); // Et on le fait sortir / terminé son service
    return 0;
}
