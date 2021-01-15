//Proiect realizat de Smau Adrian-Constantin, grupa B5 anul 2

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "include.h" // serverul are acces la structurile locale de date, clientul nu

#define PORT 2908
#define MAX_CMD_LENGTH 150

extern int errno;

typedef struct thData
{
    int idThread; //id-ul thread-ului
    int cl; //descriptorul
} thData;

static void* Treat(void*);
void Raspunde(void*);
int usr_login(void*);
int usr_register(void*);
void mainapp(void*, int);

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
void de_bug(char* str) // functie pentru debug
{
    printf("\n'%s'\n", str);
}
int main()
{
    struct sockaddr_in server;
    struct sockaddr_in from;
    int sd;
    pthread_t th[100];
    int i = 0;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) // crearea unui socker
    {
        perror("[SERVER] Eroare la socket().\n");
        return errno;
    }
    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr*)&server, sizeof(struct sockaddr)) == -1) //atasam socketul
    {
        perror("[SERVER] Eroare la bind().\n");
        return errno;
    }

    if (listen(sd, 2) == -1) //asteptam clientii
    {
        perror("[SERVER] Eroare la listen().\n");
        return errno;
    }
    printf("[SERVER] Asteptam la portul %d...\n", PORT);
    fflush(stdout);
    initStatuses(); //initializam starea tuturor clientilor drept offline
    importUsersfromDB("database.txt"); //importam local baza de date a utilizatorilor
    initInbox(); //initializam baza de date a inboxurilor
    importInboxfromDB(); //intortam local baza de date a inboxurilor
    initArchives(); //initializam arhivele conversatiilor
    printf("\nUtilizatori in baza de date : --%d--\n", countID);
    while (1)
    {
        int client;
        thData* td;
        socklen_t length = sizeof(from);
        if ((client = accept(sd, (struct sockaddr*)&from, &length)) < 0) //acceptarea unui client
        {
            perror("[SERVER] Eroare la accept().\n");
            continue;
        }

        td = (struct thData*)malloc(sizeof(struct thData));
        td->idThread = i++;
        td->cl = client;
        pthread_create(&th[i], NULL, &Treat, td);
    }
}
static void* Treat(void* arg)
{
    struct thData tdL;
    tdL = *((struct thData*)arg);
    printf("[Thread %d] Asteptam mesajul...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());
    Raspunde((struct thData*)arg);
    close((intptr_t)arg);
    return(NULL);
}
void Raspunde(void* arg)
{
    int logat = 0; //variabila ce ne indica faptul ca login-ul a fost sau nu facut
    int creat = 0; //variabila ce ne indica faptul ca inregistrarea utilizatorului a fost facuta sau nu
    int op_nmb = 0; // variabila ce ne indica daca avem de-a face cu un login, register sau quit
    struct thData tdL;
    tdL = *((struct thData*)arg);
    if (read(tdL.cl, &op_nmb, sizeof(int)) < 0) // ce operatie avem?
    {
        printf("[Thread %d]\n", tdL.idThread);
        perror("Eroare la read() de la client.\n");
    }
    /*
    op_nmb = 1 => login
    op_nmb = 2 => register

    */
    if (op_nmb == 1) //login
    {
        logat = usr_login((struct thData*)arg); // functie ce intoarce 0 daca login-ul a esuat, ID-ul userului in caz contrar
        if (logat != 0)
        {
            de_bug("Logat succesful!...");
            if (write(tdL.cl, &logat, sizeof(int)) <= 0) // trimitem ID-ul catre client
            {
                printf("[Thread %d]\n", tdL.idThread);
                perror("Eroare la read() de la client.\n");
            }
            if (write(tdL.cl, x[logat].fn, sizeof(x[logat].fn)) <= 0) // trimitem prenumele catre client
            {
                printf("[Thread %d]\n", tdL.idThread);
                perror("Eroare la read() de la client.\n");
            }
            mainapp((struct thData*)arg, logat); // trecem la secventa a doua, aplicatia in sine, retinand printr-un parametru ID-ul userului conectat cu acest thread
        }
        else
            de_bug("Logat failed!...");
    }
    else
    {
        if (op_nmb == 2)
        {
            creat = usr_register((struct thData*)arg); // functie ce intoarce 0 daca inregistrarea a esuat, ID-ul userului in caz contrar
            if (creat != 0)
            {
                de_bug("Inregistrat succesful!...");
                if (write(tdL.cl, &creat, sizeof(int)) <= 0) //trimitem ID-ul catre client
                {
                    printf("[Thread %d]\n", tdL.idThread);
                    perror("Eroare la read() de la client.\n");
                }
                if (write(tdL.cl, x[creat].fn, sizeof(x[creat].fn)) <= 0)  //trimitem prenumele catre client
                {
                    printf("[Thread %d]\n", tdL.idThread);
                    perror("Eroare la read() de la client.\n");
                }
                mainapp((struct thData*)arg, creat); // trecem la secventa a doua, aplicatia in sine, retinand printr-un parametru ID-ul userului conectat cu acest thread
            }
            else
                de_bug("Inregistrat failed!...");
        }
        else
        {
            perror("a avut loc o eroare...");
        }
    }
}
int usr_login(void* arg)
{
    char input_char[MAX_CMD_LENGTH]; //variabila de utilitate pentru citirea de la client
    bzero(&input_char, sizeof(input_char));
    char temp_user[MAX_CMD_LENGTH]; //variabila de utilitate pentru retinerea username-ului de la client
    bzero(&temp_user, sizeof(temp_user));
    int feedback = 0; //variabila de utilitate
    struct thData tdL;
    tdL = *((struct thData*)arg);
    if (read(tdL.cl, input_char, sizeof(input_char)) <= 0)  // citim usernameul pentru check
    {
        printf("[Thread %d]\n", tdL.idThread);
        perror("Eroare la read() de la client.\n");
    }
    feedback = userCheck(input_char);
    if (feedback == -1) //username invalid
    {
        de_bug("USER GRESIT!!");
        if (write(tdL.cl, &feedback, sizeof(int)) <= 0)  //feedback usercheck gresit
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
        }
        bzero(input_char, sizeof(input_char));
        return 0;
    }
    else //username valid
    {
        de_bug("USER VALID!!!");
        strcpy(temp_user, input_char);//retinem usernameul curent
        if (write(tdL.cl, &feedback, sizeof(int)) <= 0)  //dam feedback pt usercheck bun
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
        }
        bzero(input_char, sizeof(input_char));
        if (read(tdL.cl, input_char, sizeof(input_char)) <= 0)  //citim parola
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
        }
        feedback = passCheck(temp_user, input_char);
        if (feedback == -1)  //parola invalida
        {
            de_bug("PAROLA GRESITA!!!");
            if (write(tdL.cl, &feedback, sizeof(int)) <= 0)
            {
                printf("[Thread %d]\n", tdL.idThread);
                perror("Eroare la read() de la client.\n");
            }
            bzero(temp_user, sizeof(temp_user));
            bzero(input_char, sizeof(input_char));
            return 0;
        }
        else //parola valida
        {
            de_bug("PAROLA CORECTA!!!");
            if (write(tdL.cl, &feedback, sizeof(int)) <= 0)
            {
                printf("[Thread %d]\n", tdL.idThread);
                perror("Eroare la read() de la client.\n");
            }
            return (getIDbyName(temp_user)); //returnam ID-ul userului
        }
    }
}
int usr_register(void* arg)
{
    fflush(stdout);
    char input_char[MAX_CMD_LENGTH]; //variabila de utilitate pentru citirea de la client
    bzero(&input_char, sizeof(input_char));
    char temp_user[MAX_CMD_LENGTH]; //variabila de utilitate pentru retinerea username-ului de la client
    bzero(&temp_user, sizeof(temp_user));
    char temp_pass[MAX_CMD_LENGTH]; //variabila de utilitate pentru retinerea parolei de la client
    bzero(&temp_pass, sizeof(temp_pass));
    char temp_fn[MAX_CMD_LENGTH]; //variabila de utilitate pentru retinerea prenumelui de la client
    bzero(&temp_fn, sizeof(temp_fn));
    char temp_ln[MAX_CMD_LENGTH]; //variabila de utilitate pentru retinerea numelui de la client
    bzero(&temp_ln, sizeof(temp_ln));

    int feedback = 0; //variabila de utilitate
    struct thData tdL;
    tdL = *((struct thData*)arg);
    if (countID >= capacity)
    {
        feedback = -2;
        if (write(tdL.cl, &feedback, sizeof(int)) <= 0)  //feedback database full
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
        }
        return 0;
    }
    else
    {
        if (write(tdL.cl, &feedback, sizeof(int)) <= 0)  //feedback database liber
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
        }
    }
    if (read(tdL.cl, input_char, sizeof(input_char)) <= 0)  // citim usernameul pentru check
    {
        printf("[Thread %d]\n", tdL.idThread);
        perror("Eroare la read() de la client.\n");
    }
    parse_Command(input_char);
    feedback = userCheck(input_char);
    if (feedback > 0 || strcmp(input_char, "") == 0) //username folosit
    {
        de_bug("USER NEDISPONIBIL!!");
        feedback = -2;
        if (write(tdL.cl, &feedback, sizeof(int)) <= 0)  //feedback usercheck folosit
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
        }
        bzero(input_char, sizeof(input_char));
        return 0;
    }
    else //username nefolosit
    {
        de_bug("USER CREAT!!");
        if (write(tdL.cl, &feedback, sizeof(int)) <= 0)  //feedback usercheck liber
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
        }
        strcpy(temp_user, input_char);//retinem usernameul curent
        bzero(input_char, sizeof(input_char));
        if (read(tdL.cl, input_char, sizeof(input_char)) <= 0)  // citim parola
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
        }
        strcpy(temp_pass, input_char);//retinem parola curenta
        de_bug("PAROLA SETATA!!");
        bzero(input_char, sizeof(input_char));
        if (read(tdL.cl, input_char, sizeof(input_char)) <= 0)  // citim prenumele
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
        }
        strcpy(temp_fn, input_char);//retinem prenumele curent
        de_bug("PRENUME SETAT!!");
        bzero(input_char, sizeof(input_char));
        if (read(tdL.cl, input_char, sizeof(input_char)) <= 0)  // citim numele de familie
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
        }
        strcpy(temp_ln, input_char);//retinem numele curent
        de_bug("NUME SETAT!!");
        bzero(input_char, sizeof(input_char));
        if (strcmp(temp_user, "") == 0 || strcmp(temp_fn, "") == 0 || strcmp(temp_ln, "") == 0)
        {
            feedback = -1;
            if (write(tdL.cl, &feedback, sizeof(int)) <= 0)  //feedback procedura incorecta
            {
                printf("[Thread %d]\n", tdL.idThread);
                perror("Eroare la read() de la client.\n");
            }
            de_bug("Eroare...Procedura nerespectata");
            return 0;
        }
        else
        {
            feedback = 1;
            addUser(temp_user, temp_pass, temp_fn, temp_ln); //adaugam user-ul in structurile de date din cadrul serverului
            addUsertoDB("database.txt", getIDbyName(temp_user)); //adaugam user-ul la baza de date a utilizatorilor

            // adaugam entry-ul userului nou creat in baza de date a inboxurilor
            char filename[20];
            char inboxindex[3];

            bzero(inboxindex, sizeof(inboxindex));
            int ret = snprintf(inboxindex, 3, "%d", getIDbyName(temp_user));
            if (ret < 0)
            {
                abort();
            }
            bzero(filename, sizeof(filename));
            strcpy(filename, "inboxes/");
            strcat(filename, inboxindex);
            FILE* backup = fopen(filename, "a"); // nu exista, il cream
            if (backup == NULL)
            {
                perror("Eroare la deschiderea bazei de date");
                exit(1);
            }
            fclose(backup);
            initArchives(); // adaugam o conversatie intre userul nou si toti ceilalti useri din sistem
            if (write(tdL.cl, &feedback, sizeof(int)) <= 0) //feedback procedura corecta
            {
                printf("[Thread %d]\n", tdL.idThread);
                perror("Eroare la read() de la client.\n");
            }
            de_bug("USER ADAUGAT IN BAZA DE DATE!!");
            return (getIDbyName(temp_user)); //returnam ID-ul userului creat
        }
    }
}
void mainapp(void* arg, int clientID)
{
    struct thData tdL;
    tdL = *((struct thData*)arg);
    int op_nmb = 0; //variabila ce ne indica ce proces a ales clientul
    /*
        op_nmb = 1 => send
        op_nmb = 2 => inbox
        op_nmb = 3 => archive
        op_nmb = 4 => logout
        op_nmb = orice altceva => eroare neprevazuta sau greseala de sintaxa
    */
    int feedback = 0; // variabila utilitara ce ne ajuta la citirea variabilelor de tip int de la client
    char message[150]; // variabila utilitara ce ne ajuta la citirea variabilelor de tip sir de caractere de la client
    userLoggedIn(clientID); // marcam user-ul drept logat
    while (1)
    {
        if (read(tdL.cl, &op_nmb, sizeof(int)) <= 0) // ce operatie avem?
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
            op_nmb = 5; // tratam cazul in care are loc o eroare
        }
        if (op_nmb == 1) //SEND
        {
            de_bug("SUNTEM IN SEND");
            if (write(tdL.cl, &countID, sizeof(int)) <= 0) // trimitem clientului numarul de utilizatori din sistem
            {
                printf("[Thread %d]\n", tdL.idThread);
                perror("Eroare la read() de la client.\n");
            }
            char temp_status[10]; // aici retinem ACTIVE / OFFLINE, ce indica statusul fiecarui user
            char buf[150]; // produsul final prelucrat ce va fi trimis userului (pentru fiecare utilizator in parte)
            for (int i = 1; i <= countID; i++) //iteram prin useri
            {
                bzero(temp_status, sizeof(temp_status));
                if (status[x[i].ID][0] == 1)
                {
                    strcpy(temp_status, "ACTIVE");
                }
                else
                {
                    strcpy(temp_status, "OFFLINE");
                }
                bzero(buf, sizeof(buf));
                int ret = snprintf(buf, 50, "%d - %s - %s\n", x[i].ID, x[i].usr, temp_status);
                if (ret < 0)
                {
                    abort();
                }
                if (write(tdL.cl, buf, sizeof(buf)) <= 0) // trimitem datele catre client
                {
                    printf("[Thread %d]\n", tdL.idThread);
                    perror("Eroare la read() de la client.\n");
                }
            }
            if (read(tdL.cl, &feedback, sizeof(int)) < 0) // citim ID-ul userului caruia se doreste trimiterea unui mesaj
            {
                printf("[Thread %d]\n", tdL.idThread);
                perror("Eroare la read() de la client.\n");
            }
            if (feedback == 0) // ID-ul este invalid
            {
                de_bug("NE INTOARCEM IN MAIN MENU");
                continue;
            }
            else // ID-ul este valid
            {
                bzero(message, sizeof(message));
                if (read(tdL.cl, message, sizeof(message)) <= 0) // citim mesajul care se doreste trimis
                {
                    printf("[Thread %d]\n", tdL.idThread);
                    perror("Eroare la read() de la client.\n");
                }
                if (strcmp(message, "") == 0)
                {
                    de_bug("MESAJ INVALID, NU A FOST TRIMIS");
                }
                else
                {
                    sendMsg(clientID, feedback, message); // trimitem mesajul catre utilizator
                    de_bug("MESAJ TRIMIS, VOM AFISA INBOXUL FIECARUI USER");
                    showInboxes();
                    continue;
                }
            }
        }
        else
        {
            if (op_nmb == 2) //INBOX
            {
                de_bug("SUNTEM IN INBOX");
                if (write(tdL.cl, &y[clientID].msg_nmb, sizeof(int)) <= 0) // transmitem clientului numarul sau de mesaje din inbox
                {
                    printf("[Thread %d]\n", tdL.idThread);
                    perror("Eroare la read() de la client.\n");
                }
                if (y[clientID].msg_nmb == 0) // daca nu exista mesaje, ne intoarcem si asteptam alta comanda
                {
                    de_bug("Niciun mesaj in inbox...");
                    continue;
                }
                else // clientul are macar un mesaj
                {
                    for (int i = 1; i <= y[clientID].msg_nmb; i++) //iteram prin mesaje
                    {
                        if (write(tdL.cl, &y[clientID].inbox[i], sizeof(y[clientID].inbox[i])) <= 0) // afisam fiecare mesaj in parte
                        {
                            printf("[Thread %d]\n", tdL.idThread);
                            perror("Eroare la read() de la client.\n");
                        }
                    }
                    if (read(tdL.cl, &feedback, sizeof(int)) <= 0) // citim feedback ul reply-ului
                    {
                        printf("[Thread %d]\n", tdL.idThread);
                        perror("Eroare la read() de la client.\n");
                        feedback = 0; // caz in care userul se deconecteaza
                    }
                    if (feedback != 0) // userul a introdus un indice valid pentru reply
                    {
                        bzero(message, sizeof(message));
                        if (read(tdL.cl, message, sizeof(message)) <= 0) // citim mesajul cu care doreste sa raspunda
                        {
                            printf("[Thread %d]\n", tdL.idThread);
                            perror("Eroare la read() de la client.\n");
                        }
                        if (strcmp(message, "") == 0)
                        {
                            de_bug("MESAJ INVALID, NE INTOARCEM LA MAIN MENU");
                        }
                        else
                        {
                            sendMsg(clientID, y[clientID].senders[feedback], message); // trimitem celui ce a trimis mesajul cu indicele respectiv reply-ul clientului
                            de_bug("MESAJ TRIMIS, AFISAM INBOXURILE");
                            showInboxes();
                        }
                    }
                    char filename[50]; // golim inboxul clientului
                    bzero(filename, sizeof(filename));
                    int ret = snprintf(filename, 50, "inboxes/%d", clientID); // ii cream path-ul inboxului din baza de date
                    if (ret < 0)
                    {
                        abort();
                    }
                    remove(filename); // stergem fisierul
                    for (int i = 1; i <= y[clientID].msg_nmb; i++) // local, ii stergem mesajele retinute
                    {
                        bzero(y[clientID].inbox[i], sizeof(y[clientID].inbox[i]));
                    }
                    for (int i = 1; i <= y[clientID].msg_nmb; i++) // local, ii stergem ID-urile senderilor fiecarui mesaj
                    {
                        y[clientID].senders[i] = 0;
                    }
                    y[clientID].msg_nmb = 0; // local, numarul de mesaje din inbox devine 0
                    FILE* fd = fopen(filename, "a"); // recreem fisierul de inbox, care acum este gol
                    fclose(fd);
                    continue;
                }
            }
            else
            {
                if (op_nmb == 3) //archive
                {
                    de_bug("SUNTEM IN ARCHIVE");
                    if (write(tdL.cl, &countID, sizeof(int)) <= 0) // scriem numarul total de utilizatori din sistem
                    {
                        printf("[Thread %d]\n", tdL.idThread);
                        perror("Eroare la read() de la client.\n");
                    }
                    char temp_status[10]; // aici retinem statutul de ACTIV / OFFLINE al fiecarui user
                    char buf[150]; // aici se va afla linia prelucrata gata de trimis clientului
                    for (int i = 1; i <= countID; i++) // iteram prin useri
                    {
                        bzero(temp_status, sizeof(temp_status));
                        if (status[x[i].ID][0] == 1)
                        {
                            strcpy(temp_status, "ACTIVE");
                        }
                        else
                        {
                            strcpy(temp_status, "OFFLINE");
                        }
                        bzero(buf, sizeof(buf));
                        int ret = snprintf(buf, 50, "%d - %s - %s\n", x[i].ID, x[i].usr, temp_status); // prelucram linia
                        if (ret < 0)
                        {
                            abort();
                        }
                        if (write(tdL.cl, buf, sizeof(buf)) <= 0) // i-o transmitem clientului
                        {
                            printf("[Thread %d]\n", tdL.idThread);
                            perror("Eroare la read() de la client.\n");
                        }
                    }
                    if (read(tdL.cl, &feedback, sizeof(int)) <= 0) //citim cu care dintre useri se doreste a se vedea arhiva
                    {
                        printf("[Thread %d]\n", tdL.idThread);
                        perror("Eroare la read() de la client.\n");
                    }
                    if (feedback == 0) // daca numarul introdus nu este valid
                    {
                        de_bug("NE INTOARCEM LA MAIN MENU");
                        continue;
                    }
                    else
                    {
                        char temp_cmd[150];
                        bzero(temp_cmd, sizeof(temp_cmd));
                        if (read(tdL.cl, temp_cmd, sizeof(temp_cmd)) <= 0) //citim comanda
                        {
                            printf("[Thread %d]\n", tdL.idThread);
                            perror("Eroare la read() de la client.\n");
                        }
                        parse_Command(temp_cmd);
                        if (strcmp(temp_cmd, "show") == 0) // daca primim comanda show
                        {
                            bzero(buf, sizeof(buf));
                            int ret = snprintf(buf, 50, "comms/%d-%d", clientID, feedback); // prelucram path-ul arhivei din baza de date
                            if (ret < 0)
                            {
                                abort();
                            }
                            FILE* fd = fopen(buf, "r");
                            if (fd == NULL)
                            {
                                perror("Eroare la deschiderea bazei de date");
                                exit(1);
                            }
                            char current_row[150]; // variabila cu care memoram linia curenta
                            bzero(current_row, sizeof(current_row));
                            while (fgets(current_row, sizeof(current_row), fd) != NULL) // citim pana ajungem la EOF
                            {
                                current_row[strlen(current_row) - 1] = '\0';
                                if (write(tdL.cl, current_row, sizeof(current_row)) <= 0) // trimitem clientului linia curenta
                                {
                                    printf("[Thread %d]\n", tdL.idThread);
                                    perror("Eroare la read() de la client.\n");
                                }
                            }
                            bzero(current_row, sizeof(current_row));
                            strcpy(current_row, "END-OF-CONVERSATION");
                            if (write(tdL.cl, current_row, sizeof(current_row)) <= 0) // am ajuns la EOF, notificam clientul de acest lucru
                            {
                                printf("[Thread %d]\n", tdL.idThread);
                                perror("Eroare la read() de la client.\n");
                            }
                            fclose(fd);
                            continue;
                        }
                        else
                        {
                            if (strcmp(temp_cmd, "delete") == 0)
                            {
                                char filename[50]; // aici cream path-ul conversatiei
                                bzero(filename, sizeof(filename));
                                int ret = snprintf(filename, 50, "comms/%d-%d", clientID, feedback); // ii cream path-ul arhivei din baza de date
                                if (ret < 0)
                                {
                                    abort();
                                }
                                remove(filename); // stergem fisierul
                                FILE* fd = fopen(filename, "a"); // recreem fisierul de conversatie, care acum este gol
                                fclose(fd);
                                continue;
                            }
                            else
                            {
                                de_bug("Comanda nerecunoscuta");
                                continue;
                            }
                        }
                    }
                }
                else
                {
                    if (op_nmb == 4)
                    {
                        de_bug("LOGOUT EFECTUAT");
                        userLoggedOut(clientID); // delogam userul
                        break;
                    }
                    else
                    {
                        perror("A AVUT LOC O EROARE");
                        userLoggedOut(clientID); // delogam userul
                        break;
                    }
                }
            }
        }
    }
    de_bug("THREAD TERMINAT!");
    userLoggedOut(clientID); // in caz ca apare vreo eroare, delogam aici userul
}
