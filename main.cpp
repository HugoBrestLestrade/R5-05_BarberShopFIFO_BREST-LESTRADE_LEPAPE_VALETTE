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

const size_t capacite_file = 4;          // Nombre max d'étudiants dans la file d'attente
std::queue<int> file_attente;            // File d'attente devant la caisse

std::mutex tablette;                     // Tablette du CROUS : accès exclusif à la file d'attente
std::condition_variable cv_employe;      // Pour sortir l'employé de TikTok
std::condition_variable cv_etudiants;    // Pour sortir les étudiants de TikTok
int etudiant_appele = -1;                // Numéro de l'étudiant appelé par l'employé
std::atomic<bool> cafeteria_fermee(false); // Atomique : lu hors verrou dans la boucle de l'employé

std::binary_semaphore commande_servie(0);        // Jeton : l'employé indique à l'étudiant que sa commande est prête
std::binary_semaphore place_paiement_liberee(0); // Jeton : l'étudiant indique qu'il a quitté la place de paiement

void employe_crous(){
    while (!cafeteria_fermee){
        std::unique_lock<std::mutex> verrou(tablette); // L'employé consulte la tablette

        // Tant que la file est vide et que la cafétéria est ouverte, l'employé scrolle sur TikTok
        if (file_attente.empty() && !cafeteria_fermee)
            cout << "L'employé du CROUS n'a personne : il scrolle sur TikTok." << endl;
        cv_employe.wait(verrou, [&] { return !file_attente.empty() || cafeteria_fermee; });

        // Si la cafétéria est ouverte, l'employé appelle le premier étudiant de la file
        if (!cafeteria_fermee) {
            int id_etudiant = file_attente.front();
            file_attente.pop();                 // Une place se libère dans la file
            etudiant_appele = id_etudiant;

            verrou.unlock(); // L'employé repose la tablette avant de servir

            cv_etudiants.notify_all(); // Réveille les étudiants pour que le bon se manifeste

            cout << "L'employé prend la commande de l'étudiant " << id_etudiant << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            cout << "L'étudiant " << id_etudiant << " récupère sa commande et quitte la cafétéria." << endl;

            commande_servie.release();          // Remet la commande à l'étudiant
            place_paiement_liberee.acquire();   // Attend que la place de paiement soit libre
        }
    }
}

void etudiant(int id){
    std::unique_lock<std::mutex> verrou(tablette); // L'étudiant prend la tablette
    if (file_attente.size() < capacite_file){       // La file n'est pas pleine
        file_attente.push(id);                      // Signale sa présence sur la tablette
        cout << "L'étudiant " << id << " entre, se signale sur la tablette et scrolle sur TikTok." << endl;
        cv_employe.notify_one();                    // Sort l'employé de TikTok s'il y est

        // Tant que ce n'est pas mon numéro, je continue de scroller
        cv_etudiants.wait(verrou, [&] { return etudiant_appele == id; });
        verrou.unlock();

        commande_servie.acquire();          // Attend sa commande à la place de paiement
        place_paiement_liberee.release();   // Libère la place de paiement
    }
    else{
        cout << "L'étudiant " << id << " voit à travers la vitre que la file est pleine et repart." << endl;
    }
}

int main()
{
    std::thread t_employe(employe_crous);
    std::vector<std::thread> etudiants;

    for (int i = 0; i < 20; ++i){
        etudiants.emplace_back(etudiant, i);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Un étudiant arrive toutes les secondes
    }

    // On attend que tous les étudiants soient servis ou repartis
    for (std::thread & t : etudiants)
        t.join();

    // Plus aucun étudiant : on ferme la cafétéria et on sort l'employé de TikTok pour qu'il parte
    {
        std::lock_guard<std::mutex> verrou(tablette); // Verrou RAII : libéré automatiquement à la fin du bloc
        cafeteria_fermee = true;
    }
    cv_employe.notify_one();

    t_employe.join();
    return 0;
}
