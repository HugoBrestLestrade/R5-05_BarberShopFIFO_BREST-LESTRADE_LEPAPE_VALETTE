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
queue<int> file_attente; // File d'attente devant la caisse

mutex tablette; // Tablette du CROUS, représente l'accès exclusif à la file d'attente

//condition_variable reveil_employe;
counting_semaphore<100> reveil_employe(0);// Pour sortir l'employé de TikTok, on passe de la variable de condition à un sémaphore pour pouvoir réaliser l'interblocage


condition_variable reveil_etudiants; // Pour sortir les étudiants de TikTok lorsqu'un étudiant est appellé
int etudiant_appele = -1; // Numéro de l'étudiant appelé par l'employé != étudiant au hasard qui serait le problème classique du barbershop
atomic<bool> cafeteria_fermee(false);

binary_semaphore commande_servie(0); // Jeton pour que l'employé indique à l'étudiant que sa commande est prête
binary_semaphore place_paiement_liberee(0); // Jeton pour que l'étudiant indique qu'il a quitté la place de paiement

void employe_crous(){
    while (!cafeteria_fermee){
        unique_lock<mutex> acces_tablette(tablette); // L'employé consulte la tablette

        // Tant que la file est vide et que la cafétéria est ouverte, l'employé scrolle sur TikTok
        if (file_attente.empty() && !cafeteria_fermee){
            cout << endl << "\033[34m L'employé du CROUS \033[0m a vérifié sur la tablette, il n'y a personne, il scrolle donc sur TikTok." << endl;
            cout << endl << "\033[33m Il se met a scroller sur la tablette au lieu de son téléphone...\033[0m" << endl << endl;
            //reveil_employe.wait(acces_tablette, [&] { return !file_attente.empty() || cafeteria_fermee; });

        }
        reveil_employe.acquire();



        // Si la cafétéria est ouverte, l'employé appelle le premier étudiant de la file
        if (!cafeteria_fermee) {
            int id_etudiant = file_attente.front(); // Récupère l'identifiant du premier étudiant de la file
            file_attente.pop(); // Puis le fait sortir de la file d'attente (l'étudiant scrolle encore)
            etudiant_appele = id_etudiant; // L'étudiant qui va se faire servir est celui qui vient d'être sorti de la file

            acces_tablette.unlock(); // L'employé repose la tablette avant de servir l'étudient ( On libère le mutex )

            reveil_etudiants.notify_all(); // Réveille les étudiants pour que le bon se manifeste

            cout << endl <<"\033[34m L'employé \033[0m prend la commande de l'étudiant \033[33m" << id_etudiant << "\033[0m" << endl << endl;
            this_thread::sleep_for(chrono::milliseconds(2000)); // 2s = le temps que l'employé serve l'étudiant
            commande_servie.release(); // Remet la commande à l'étudiant
            place_paiement_liberee.acquire(); // Attend que la place de paiement soit libre
            cout << "\033[32m L'étudiant \033[33m" << id_etudiant << "\033[0m récupère sa commande et quitte la cafétéria." << endl << endl;
        }
    }
}

void etudiant(int id){

    cout << "\033[32m L'étudiant \033[33m" << id << "\033[0m arrive devant la cafétéria et essaie de prendre la tablette" << endl << endl;
    unique_lock<mutex> acces_tablette(tablette); // L'étudiant prend la tablette
    if (file_attente.size() < capacite_file){ // Vérifie que la file n'est pas pleine
        file_attente.push(id); // Signale sa présence sur la tablette
        cout << "\033[32m L'étudiant \033[33m" << id << "\033[0m entre, se signale sur la tablette et scrolle sur TikTok." << endl << endl;


        //reveil_employe.notify_one(); // Sort l'employé de TikTok s'il y est

        reveil_employe.release();


        // Tant que ce n'est pas le numéro de l'étudiant, il continue de scroller
        reveil_etudiants.wait(acces_tablette, [&] { return etudiant_appele == id; });

        acces_tablette.unlock();
        commande_servie.acquire(); // Attend sa commande à la place de paiement
        place_paiement_liberee.release(); // Libère la place de paiement
    }
    else{
        cout << "\033[32m L'étudiant \033[33m" << id << "\033[0m voit à travers la vitre que la file est pleine.\033[31m Il repart très énervé \033[0m" << endl << endl;
    }
}

int main()
{
    vector<thread> threads_etudiants;

    cout << endl << "\033[34mOuverture de la cafétéria !\033[0m" << endl;
    cout << "\033[33mLe nombre de places dans la file d'attente de la cafétéria est actuellement de 4\033[0m" << endl << endl;

    cout << "\033[31m3 étudiants se ruent à l'intérieur, ils sont affamés\033[0m" << endl << endl;

    // Les 3 étudiants rentrent rapidement
    for (int i = 0; i < 3; ++i){
        threads_etudiants.emplace_back(etudiant, i);
    }

    this_thread::sleep_for(chrono::milliseconds(3000));

    // Arrivée régulière des étudiants
    thread thread_employe(employe_crous);
    for (int i = 3; i < 10; ++i){
        threads_etudiants.emplace_back(etudiant, i);
        this_thread::sleep_for(chrono::milliseconds(1000)); // Un étudiant arrive toutes les secondes
    }

    // On attend que tous les étudiants soient servis ou repartis
    for (thread & t : threads_etudiants)
        if (t.joinable()){
            t.join();
        }

    cout <<" Quelques étudiants entrent dans la cafétéria, l'employé scrolle actuellement sur TikTok." << endl << endl;

    for (int i = 10; i < 16; ++i){
        threads_etudiants.emplace_back(etudiant, i);
        this_thread::sleep_for(chrono::milliseconds(1000)); // Un étudiant arrive toutes les secondes
    }

    // On attend que tous les étudiants soient servis ou repartis
    for (thread & t : threads_etudiants){
        if (t.joinable()){
            t.join();
        }
    }

    // On ferme la cafétéria car il n'y a plus aucun étudiant
    {
        lock_guard<mutex> acces_tablette(tablette);
        cafeteria_fermee = true;
    }

    // On sort l'employé de TikTok une dernière fois pour lui dire que la cafétéria est fermée
    reveil_employe.release();

    thread_employe.join(); // Et on le fait sortir / terminer son service

    cout << " C'est 17h30, \033[34m l'employé \033[0m a fini son travail et rentre chez lui, il ferme la cafétéria" << endl;
    cout << endl << "\033[33m La cafétéria ferme...\033[0m" << endl << endl;
    return 0;
}
