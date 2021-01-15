//Proiect realizat de Smau Adrian-Constantin, grupa B5 anul 2

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

#define MAX_CMD_LENGTH 150
extern int errno;

int port;

void parse_Command(char* cmd)  //Eliminam spatiile de la inceputul si sfarsitul comenzii
{
    int front = 0;
    int end = 0;
    int last_char = strlen(cmd) - 1;
    while (cmd[front] == ' ' || cmd[front] == '\t')   //numaram whitespace-urile si tab-urile de la inceput
    {
        front++;
    }
    while (cmd[last_char] == ' ' || cmd[last_char] == '\n' || cmd[last_char] == '\t')   //numaram whitespace-urile si tab-urile de la final
    {
        end++;
        last_char--;
    }
    int j = 0;
    for (int i = front; (size_t)i < (strlen(cmd) - end); i++)   //iteram intre whitespace-urile si tab-urile de la inceput si final si recreem cmd de la inceput
    {
        cmd[j++] = cmd[i];
    }
    cmd[j] = '\0'; //inchidem cmd punand pe ultima pozitie un '\0'
}
int main(int argc, char* argv[])
{
    int sd; // descriptorul prin intermediul caruia comunicam cu serverul
    struct sockaddr_in server;
    char msg[MAX_CMD_LENGTH]; // variabila folosita pentru citirea din stdin
    int op_nmb = 0,/* ne ajuta sa identificam ce comanda a ales user-ul */ feedback1 = 0 /* variabila de utilitate pe care o folosim la citirea de la server */;

    if (argc != 3)
    {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    port = atoi(argv[2]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[CLIENT] Eroare la socket().\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr*)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[CLIENT] Eroare la connect().\n");
        return errno;
    }

    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\nBun venit! Pentru a continua, trebuie sa te autentifici. Daca nu ai cont, te poti inregistra. Sintaxa comenzilor este urmatoarea :\n <login>, pentru a te loga in contul de utilizator deja existent\n <register>, pentru a-ti crea un nou cont de utilizator\n <quit>, pentru a iesi din program\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    fflush(stdout);
    int logged = 0; // variabila ce ne va ajuta sa determinam daca user-ul este logat


// PRIMA SECVENTA -> LOGIN


    while (1) // LOGIN SEQUENCE
    {
        bzero(&msg, sizeof(msg));
        fgets(msg, MAX_CMD_LENGTH, stdin); // citim comanda
        parse_Command(msg);
        /*
            op_nmb = 1 => login
            op_nmb = 2 => register

        */
        if (strcmp(msg, "login") == 0)
        {
            op_nmb = 1;
            if (write(sd, &op_nmb, sizeof(int)) <= 0)  //am ales optiunea login
            {
                perror("[CLIENT] Eroare la write() spre server.\n");
                return errno;
            }
            printf("Va rog introduceti numele dvs. de utilizator : ");
            fflush(stdout);
            bzero(&msg, sizeof(msg));
            fgets(msg, MAX_CMD_LENGTH, stdin);
            parse_Command(msg);

            if (write(sd, msg, sizeof(msg)) <= 0)  //trimitem username pentru check
            {
                perror("[CLIENT] Eroare la write() spre server.\n");
                return errno;
            }
            if (read(sd, &feedback1, sizeof(int)) <= 0)  //feedback usercheck
            {
                perror("[CLIENT] Eroare la read() de la server.\n");
                return errno;
            }
            if (feedback1 == -1)  //usercheck failed
            {
                printf("\nUsername-ul introdus de dvs. nu se afla in baza noastra de date. Va rugam incercati din nou sau inregistrati-va cu un cont nou!...\nDisconnecting...\n");
                break;
            }
            else  //continuam cu parola
            {
                printf("Va rog introduceti parola : ");
                fflush(stdout);
                bzero(&msg, sizeof(msg));
                fgets(msg, MAX_CMD_LENGTH, stdin);
                parse_Command(msg);

                if (write(sd, msg, sizeof(msg)) <= 0)  //trimitem parola pentru check
                {
                    perror("[CLIENT] Eroare la write() spre server.\n");
                    return errno;
                }
                if (read(sd, &feedback1, sizeof(int)) < 0)  //feedback passcheck
                {
                    perror("[CLIENT] Eroare la read() de la server.\n");
                    return errno;
                }
                if (feedback1 == -1)
                {
                    printf("\nParola incorecta! Va rugam incercati din nou...\nDisconnecting...\n");
                    break;
                }
                else
                {
                    printf("Login efectuat cu succes!\n\nVei fi redirectat la Main Menu!...\n\n");
                    logged = 1; //marcam user-ul drept logat
                    break;
                }

            }
        }
        else
        {
            if (strcmp(msg, "register") == 0)
            {
                op_nmb = 2;
                if (write(sd, &op_nmb, sizeof(int)) <= 0)  //am ales optiunea register
                {
                    perror("[CLIENT] Eroare la write() spre server.\n");
                    return errno;
                }
                if (read(sd, &feedback1, sizeof(int)) < 0)  //vedem daca mai avem loc in database
                {
                    perror("[CLIENT] Eroare la read() de la server.\n");
                    return errno;
                }
                if (feedback1 == -2)
                {
                    printf("Limita de utilizatori pe aceasta platforma a fost atinsa...\nDisconnecting...\n");
                    break;
                }
                printf("Incepe inregistrarea prin a-ti seta un nume de utilizator : ");
                fflush(stdout);
                bzero(&msg, sizeof(msg));
                fgets(msg, MAX_CMD_LENGTH, stdin);
                parse_Command(msg);

                if (write(sd, msg, sizeof(msg)) <= 0)  //trimitem username pentru a vedea daca se afla deja in baza de date
                {
                    perror("[CLIENT] Eroare la write() spre server.\n");
                    return errno;
                }
                if (read(sd, &feedback1, sizeof(int)) <= 0)  //feedback usercheck
                {
                    perror("[CLIENT] Eroare la read() de la server.\n");
                    return errno;
                }
                if (feedback1 == -1)  //username ul nu este folosit
                {
                    printf("Mai departe, va rog scrieti o parola usor de retinut cu care va veti loga in cont : ");
                    fflush(stdout);
                    bzero(&msg, sizeof(msg));
                    fgets(msg, MAX_CMD_LENGTH, stdin);
                    parse_Command(msg);

                    if (write(sd, msg, sizeof(msg)) <= 0)  //trimitem parola
                    {
                        perror("[CLIENT] Eroare la write() spre server.\n");
                        return errno;
                    }
                    printf("Introduceti prenumele dvs. : ");
                    fflush(stdout);
                    bzero(&msg, sizeof(msg));
                    fgets(msg, MAX_CMD_LENGTH, stdin);
                    parse_Command(msg);

                    if (write(sd, msg, sizeof(msg)) <= 0)  //trimitem prenumele
                    {
                        perror("[CLIENT] Eroare la write() spre server.\n");
                        return errno;
                    }
                    printf("Introduceti numele dvs. : ");
                    fflush(stdout);
                    bzero(&msg, sizeof(msg));
                    fgets(msg, MAX_CMD_LENGTH, stdin);
                    parse_Command(msg);

                    if (write(sd, msg, sizeof(msg)) <= 0)  //trimitem numele
                    {
                        perror("[CLIENT] Eroare la write() spre server.\n");
                        return errno;
                    }
                    if (read(sd, &feedback1, sizeof(int)) < 0)
                    {
                        perror("[CLIENT] Eroare la read() de la server.\n");
                        return errno;
                    }
                    if (feedback1 == -1)
                    {
                        printf("Procedura de inregistrare nu a fost respectata!\nDisconnecting...\n");
                        break;
                    }
                    else
                    {
                        printf("Cont creat cu succes!\n\nVei fi redirectat la Main Menu!...\n\n");
                        logged = 1;
                        break;
                    }
                }
                else  //username luat
                {
                    printf("Numele de utilizator se afla deja in baza noastra de date sau a aparut o eroare! Va rugam incercati din nou...\nDisconnecting...\n");
                    break;
                }
            }
            else
            {
                if (strcmp(msg, "quit") == 0)
                {
                    printf("Disconnecting...\n");
                    break;
                }
                else
                {
                    printf("Sintaxa incorecta!...Va rugam incercati din nou! \n");
                    continue;
                }
            }
        }
    }
    if (logged == 0)
        close(sd);


    //Sfarsitul seccventei de login
    //Inceputul secventei de mainapp

    //A DOUA SECVENTA -> MAINAPP


    else
    {
        int myID = 0; // ID-ul clientului curent
        char myName[100]; // prenumele clientului curent
        char temp_row[150]; // variabila utilizata folosita pe parcursul functiei pentru diverse procese pe baza de siruri de caractere
        int usernumber = 0; // variabila utilizata folosita pe parcursul functiei pentru diverse procese pe baza de numere
        if (read(sd, &myID, sizeof(int)) <= 0) //citim ID-ul userului conectat
        {
            perror("[CLIENT] Eroare la read() de la server.\n");
            return errno;
        }
        if (read(sd, myName, sizeof(myName)) <= 0) //citim prenumele userului conectat
        {
            perror("[CLIENT] Eroare la read() de la server.\n");
            return errno;
        }
        while (1)
        {
            printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\nBun venit pe platforma Offline Messenger, %s!\nSintaxa comenzilor este urmatoarea :\n<send>, pentru a-i trimite unui utilizator un mesaj\n<inbox>, pentru a verifica daca ai mesaje noi\n<archive>, pentru a vedea/sterge istoricul conversatiilor cu alti utilizatori\n<logout>, pentru a iesi din program\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n", myName);
            bzero(&msg, sizeof(msg));
            fgets(msg, MAX_CMD_LENGTH, stdin);
            parse_Command(msg);
            /*
                op_nmb = 1 => send
                op_nmb = 2 => inbox
                op_nmb = 3 => archive
                op_nmb = 4 => logout
                op_nmb = orice altceva => eroare neprevazuta sau greseala de sintaxa
            */
            if (strcmp(msg, "send") == 0)
            {
                printf("Aceasta este lista utilizatorilor din baza de date:\n");
                op_nmb = 1;
                if (write(sd, &op_nmb, sizeof(int)) <= 0)  //trimitem indicele corespunzator operatiei alese
                {
                    perror("[CLIENT] Eroare la write() spre server.\n");
                    return errno;
                }
                if (read(sd, &usernumber, sizeof(int)) <= 0) //citim numarul total de utilizatori
                {
                    perror("[CLIENT] Eroare la read() de la server.\n");
                    return errno;
                }
                for (int i = 1; i <= usernumber; i++) //pentru toti utilizatorii din sistem
                {
                    bzero(temp_row, sizeof(temp_row));
                    if (read(sd, temp_row, sizeof(temp_row)) <= 0) //afisam ID-ul, username-ul si starea de conectare prelucrate in server
                    {
                        perror("[CLIENT] Eroare la read() de la server.\n");
                        return errno;
                    }
                    printf("%s", temp_row);
                }
                printf("Alegeti utilizatorul caruia doriti sa ii scrieti un mesaj prin a-i scrie ID-ul mai jos!\n");
                bzero(msg, sizeof(msg));
                fgets(msg, MAX_CMD_LENGTH, stdin);
                parse_Command(msg);
                if (strcmp(msg, "") == 0) //string invalid
                    feedback1 = -1;
                else
                    feedback1 = atoi(msg);
                if (feedback1 > usernumber || feedback1 < 0 || feedback1 == myID) // ID invalid
                {
                    printf("\nID invalid...va rugam sa incercati din nou! Veti fi redirectionati catre meniu!\n\n");
                    feedback1 = 0;
                    if (write(sd, &feedback1, sizeof(int)) <= 0)  //am ales optiunea send
                    {
                        perror("[CLIENT] Eroare la write() spre server.\n");
                        return errno;
                    }
                    continue;
                }
                else // ID valid
                {
                    if (write(sd, &feedback1, sizeof(int)) <= 0)  //transmited ID-ul utilizatorului caruia dorim sa ii dam mesaj
                    {
                        perror("[CLIENT] Eroare la write() spre server.\n");
                        return errno;
                    }
                    printf("Acum, scrieti mesajul pe care doriti sa il trimiteti!\n");
                    bzero(&msg, sizeof(msg));
                    fgets(msg, MAX_CMD_LENGTH, stdin);
                    parse_Command(msg);
                    if (write(sd, msg, sizeof(msg)) <= 0)  //scriem mesajul pe care dorim sa il trimitem
                    {
                        perror("[CLIENT] Eroare la write() spre server.\n");
                        return errno;
                    }
                    if (strcmp(msg, "") == 0)
                    {
                        printf("\nMesajul introdus de dumneavoastra este invalid. Mesajul nu a fost trimis, veti fi redirectionat la meniul principal!..\n\n");
                        continue;
                    }
                    else
                    {
                        printf("\nMesaj trimit cu succes! Veti fi redirectionat la meniul principal!..\n\n");
                        continue;
                    }
                }
            }
            else
            {
                if (strcmp(msg, "inbox") == 0)
                {
                    op_nmb = 2;
                    if (write(sd, &op_nmb, sizeof(int)) <= 0) //trimitem indicele corespunzator operatiei alese
                    {
                        perror("[CLIENT] Eroare la write() spre server.\n");
                        return errno;
                    }
                    if (read(sd, &usernumber, sizeof(int)) <= 0) //citim numarul de mesaje din inbox de la server
                    {
                        perror("[CLIENT] Eroare la read() de la server.\n");
                        return errno;
                    }
                    if (usernumber == 0) // nu avem mesaje
                    {
                        printf("\nNu aveti mesaje in inbox. Veti fi redirectionat catre meniul principal...\n\n");
                        continue;
                    }
                    else // avem macar un mesaj
                    {
                        printf("\nAveti (%d) mesaje noi : \n\n", usernumber);
                        char temp_msg[150];
                        for (int i = 1; i <= usernumber; i++) //iteram prin toate mesajele din inbox
                        {
                            bzero(temp_msg, sizeof(temp_msg));
                            if (read(sd, &temp_msg, sizeof(temp_msg)) <= 0) // afisam fiecare mesaj in parte
                            {
                                perror("[CLIENT] Eroare la read() de la server.\n");
                                return errno;
                            }
                            printf("%s\n", temp_msg);
                        }
                        printf("\nIntroduceti indexul mesajului la care doriti sa raspundeti in cazul in care doriti sa faceti acest lucru, sau comanda <back> daca doriti sa fiti redirectionati in meniul principal. Inboxul dumneavoastra va fi golit!\n");
                        bzero(msg, sizeof(msg));
                        fgets(msg, MAX_CMD_LENGTH, stdin); // citim din stdin indexul mesajului la care dorim sa facem reply, sau back daca nu dorim acest lucru
                        parse_Command(msg);
                        if (strcmp(msg, "back") != 0 && strcmp(msg, "") != 0) // introducem un index
                        {
                            feedback1 = atoi(msg);
                            if (feedback1 < 0 || feedback1 > usernumber)
                            {
                                feedback1 = 0;
                                if (write(sd, &feedback1, sizeof(int)) <= 0) // trimitem 0 pentru a indica faptul ca nu dorim sa facem reply
                                {
                                    perror("[CLIENT] Eroare la write() spre server.\n");

                                }
                                printf("Indicele introdus de dumneavoastra nu este valid. Veti fi redirectionat catre meniul principal!...\n\n");
                                continue;
                            }
                            else
                            {
                                if (write(sd, &feedback1, sizeof(int)) <= 0) // trimitem serverului indexul ales
                                {
                                    perror("[CLIENT] Eroare la write() spre server.\n");
                                }
                                printf("Va rog introduceti mesajul cu care vreti sa raspundeti!...\n");
                                bzero(msg, sizeof(msg));
                                fgets(msg, MAX_CMD_LENGTH, stdin); // citim din stdin mesajul cu care dorim sa raspundem
                                parse_Command(msg);
                                if (write(sd, msg, sizeof(msg)) <= 0)  //trimitem mesajul catre server
                                {
                                    perror("[CLIENT] Eroare la write() spre server.\n");
                                    return errno;
                                }
                                if (strcmp(msg, "") == 0)
                                {
                                    printf("\nMesajul inserat nu este valid si nu a fost trimis. Veti fi redirectionat catre meniul principal!...\n");
                                }
                                else
                                {
                                    printf("\nMesaj trimis cu succes! Veti fi redirectionat la meniul principal!..\n");
                                }
                                continue;
                            }
                        }
                        else // am ales optiunea back
                        {
                            feedback1 = 0;
                            if (write(sd, &feedback1, sizeof(int)) <= 0) // trimitem 0 pentru a indica faptul ca nu dorim sa facem reply
                            {
                                perror("[CLIENT] Eroare la write() spre server.\n");

                            }
                            if (strcmp(msg, "") == 0)
                                printf("\nIndex invalid. Veti fi redirectat catre meniul principal!...\n\n");
                            else
                                printf("\nOptiunea <back> selectata. Veti fi redirectionat catre meniul principal!...\n\n");
                            continue;
                        }
                    }
                }
                else
                {
                    if (strcmp(msg, "archive") == 0)
                    {
                        op_nmb = 3;
                        if (write(sd, &op_nmb, sizeof(int)) <= 0) //trimitem indicele corespunzator operatiei alese
                        {
                            perror("[CLIENT] Eroare la write() spre server.\n");
                            return errno;
                        }
                        printf("Aceasta este lista utilizatorilor din baza de date:\n");
                        if (read(sd, &usernumber, sizeof(int)) <= 0) // citim numarul total de utilizatori din sistem
                        {
                            perror("[CLIENT] Eroare la read() de la server.\n");
                            return errno;
                        }
                        for (int i = 1; i <= usernumber; i++)
                        {
                            bzero(temp_row, sizeof(temp_row));
                            if (read(sd, temp_row, sizeof(temp_row)) <= 0) // pentru fiecare, citim ID-ul usernameul si statusul de conectare prelucrate in server
                            {
                                perror("[CLIENT] Eroare la read() de la server.\n");
                                return errno;
                            }
                            printf("%s", temp_row);
                        }
                        printf("Alegeti utilizatorul cu care doriti sa vedeti arhiva prin a-i scrie ID-ul mai jos!\n");
                        fflush(stdout);
                        bzero(msg, sizeof(msg));
                        fgets(msg, MAX_CMD_LENGTH, stdin); // citim din stdin ID-ul utilizatorului cu care dorim sa vedem/stergem conversatia
                        parse_Command(msg);
                        if (strcmp(msg, "") == 0) // id invalid
                            feedback1 = -1;
                        else
                            feedback1 = atoi(msg);
                        if (feedback1 > usernumber || feedback1 < 0 || feedback1 == myID) // verificam validitatea ID-ului
                        {
                            printf("\nID invalid...va rugam sa incercati din nou! Veti fi redirectionati catre meniu!\n\n");
                            feedback1 = 0;
                            if (write(sd, &feedback1, sizeof(int)) <= 0) // trimitem 0 pentru a indica o eroare
                            {
                                perror("[CLIENT] Eroare la write() spre server.\n");
                                return errno;
                            }
                            continue;
                        }
                        else
                        {
                            if (write(sd, &feedback1, sizeof(int)) <= 0) // trimitem ID-ul userului cu care dorim sa vedem / sterge conversatia
                            {
                                perror("[CLIENT] Eroare la write() spre server.\n");
                                return errno;
                            }
                            printf("Daca doriti sa vizualizati conversatia cu utilizatorul mentionat, introduceti comanda <show>. Daca doriti sa stergeti aceasta conversatie, introduceti comanda <delete>\n");
                            fflush(stdout);
                            bzero(msg, sizeof(msg));
                            fgets(msg, MAX_CMD_LENGTH, stdin);
                            parse_Command(msg);
                            if (write(sd, msg, sizeof(msg)) <= 0) //trimitem comanda aleasa catre server
                            {
                                perror("[CLIENT] Eroare la write() spre server.\n");
                                return errno;
                            }
                            if (strcmp(msg, "show") == 0)
                            {
                                printf("\nVa fi afisata arhiva convorbirilor cu utilizatorul cu ID-ul : %d\n\n", feedback1);
                                while (strcmp(temp_row, "END-OF-CONVERSATION") != 0) // citim cat timp nu primim mesajul "END-OF-CONVERSATION" care indica capatul arhivei
                                {
                                    bzero(temp_row, sizeof(temp_row));
                                    if (read(sd, temp_row, sizeof(temp_row)) <= 0) // citim mesajul si il afisam
                                    {
                                        perror("[CLIENT] Eroare la read() de la server.\n");
                                        return errno;
                                    }
                                    printf("%s\n", temp_row);
                                }
                                printf("\nVeti fi redirectionat catre meniul principal!...\n");
                                continue;
                            }
                            else
                            {
                                if (strcmp(msg, "delete") == 0)
                                {
                                    printf("\nConversatia cu utilizatorul cu ID-ul %d a fost stearsa cu succes!\n\n", feedback1);
                                    continue;

                                }
                                else
                                {
                                    printf("\nComanda nerecunoscuta! Veti fi redirectionat catre meniul principal!\n\n");
                                    continue;
                                }
                            }
                        }
                    }
                    else
                    {
                        if (strcmp(msg, "logout") == 0)
                        {
                            op_nmb = 4;
                            if (write(sd, &op_nmb, sizeof(int)) <= 0) //trimitem indicele corespunzator operatiei alese
                            {
                                perror("[CLIENT] Eroare la write() spre server.\n");
                                return errno;
                            }
                            printf("Urmeaza sa fiti deconectati!...\n");
                            break;
                        }
                        else
                        {
                            printf("Sintaxa incorecta!...Va rugam incercati din nou! \n");
                            continue;
                        }
                    }
                }
            }
        }
    }
    close(sd);
}
