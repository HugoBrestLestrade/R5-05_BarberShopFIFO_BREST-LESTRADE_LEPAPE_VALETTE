### Résultats attendus pour les tests : 

#### Solution_FIFO.cpp

Ce fichier représente l'exécution normale de notre solution sans interblocage.
Il comprend 2 tests différents, le premier pour une exécution avec une arrivée régulière d'étudiants, et le deuxième pour une arrivée irrégulière d'étudiants.

##### Comportement attendu du test 1 : 

Les étudiants arrivent toutes les secondes, l'employé du CROUS doit les gérer au fur et à mesure.
Les étudiants s'endorment une fois dans la file, ils scrollent sur TikTok.
Si la file est pleine, les étudiants qui arrivent sont censés repartir immédiatement.

##### Comportement attendu du test 2 :

Les étudiants arrivent de manière irrégulière, dont plusieurs très rapidement au début, le code doit les gérer progressivement.
Lorsque tous les étudiants ont été servis, l'employé du CROUS s'endort et scrolle sur TikTok.
Si un étudiant arrive à ce moment-là, il doit se réveiller et aller le gérer.

#### interblocage.cpp

Ce fichier contient le problème d'interblocage, il respecte les 4 conditions de Coffman.
Le test gère de la même manière que dans le fichier Solution_FIFO.cpp l'arrivée simultanée dans la file, ainsi que le rejet d'étudiants si la file est pleine.
Mais si l'employé du CROUS s'endort, il doit oublier de libérer le mutex, et doit scroller avec la tablette, empêchant ainsi les étudiants d'accéder à la file.
Cela doit provoquer un interblocage.

