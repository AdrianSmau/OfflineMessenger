//Proiect realizat de Smau Adrian-Constantin, grupa B5 anul 2

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_USERS 90
#define INBOX_LIMIT 200
int countID = 0; // numarul de utilizatori din sistem
int capacity = MAX_USERS; // capacitatea sistemului

typedef struct User_info // structura de date ce retine local userii
{
    int ID;
    char usr[100], pwd[100], fn[100], ln[100];
} User_info;

User_info x[MAX_USERS];

typedef struct inbox // structura de date ce retine inboxurile local
{
    int ID;
    int msg_nmb; // numarul de mesaje din inbox
    char inbox[INBOX_LIMIT][150]; // mesajele din inbox
    int senders[INBOX_LIMIT]; // ID-ul celor ce au trimis respectivele mesaje
} inbox;

inbox y[MAX_USERS];

int status[MAX_USERS][1]; // structura de date ce retine local statusul fiecarui user (LOGAT/OFFLINE) => 0 = OFFLINE / 1 = ONLINE


//ARCHIVE operations


void initArchives()
{
    // initializam arhivele
    char filename[50];
    for (int i = 1; i <= countID; i++)
    {
        for (int j = 1; j <= countID; j++)
        {
            if (i != j)
            {
                bzero(filename, sizeof(filename));
                int ret = snprintf(filename, 50, "comms/%d-%d", i, j); // cream pathul fiecarei arhive
                if (ret < 0)
                {
                    abort();
                }
                FILE* fd = fopen(filename, "a"); // o cream, daca nu exista deja
                fclose(fd);
            }
        }
    }
}
void writeToArchiveSR(int IDsender, int IDreceiver, char* message) // cand trimitem un mesaj, il adaugam in arhiva de la sender la receiver
{
    char filename[50];
    int ret = snprintf(filename, 50, "comms/%d-%d", IDsender, IDreceiver);
    if (ret < 0)
    {
        abort();
    }
    FILE* fd = fopen(filename, "a");
    fprintf(fd, "[%s %s] - %s\n", x[IDsender].fn, x[IDsender].ln, message);
    fclose(fd);
}
void writeToArchiveRS(int IDsender, int IDreceiver, char* message) // cand trimitem un mesaj, il adaugam in arhiva de la receiver la sender
{
    char filename[50];
    int ret = snprintf(filename, 50, "comms/%d-%d", IDreceiver, IDsender);
    if (ret < 0)
    {
        abort();
    }
    FILE* fd = fopen(filename, "a");
    fprintf(fd, "[%s %s] - %s\n", x[IDsender].fn, x[IDsender].ln, message);
    fclose(fd);
}
void initInbox() // initializam local numarul de mesaje din inboxuri
{
    for (int j = 1; j <= countID; j++)
        y[j].msg_nmb = 0;
}
void writeToInboxDB(int IDreceiver, char* message) // cand trimitem un mesaj sau un reply, il adaugam in baza de date a inboxurilor
{
    char filename[50];
    int ret = snprintf(filename, 50, "inboxes/%d", IDreceiver);
    if (ret < 0)
    {
        abort();
    }
    FILE* fd = fopen(filename, "a");
    fprintf(fd, "%s", message);
    fclose(fd);
}


//SEND operation


void sendMsg(int IDsender, int IDreceiver, char* message) // functia de trimis mesaj
{
    if (y[IDreceiver].msg_nmb < INBOX_LIMIT)
    {
        y[IDreceiver].msg_nmb++; // local, numarul de mesaje ale receiverului creste
        int msgindex = y[IDreceiver].msg_nmb;
        char temp_msg[150]; // aici vom prelucra mesajul pentru a fi adaugat in inbox / arhiva
        bzero(temp_msg, sizeof(temp_msg));
        strcpy(temp_msg, "(");
        char temp_nmb[10]; // aici retinem indexul mesajului pentru a fi adaugat in inbox
        bzero(temp_nmb, sizeof(temp_nmb));
        int ret = snprintf(temp_nmb, 10, "%d", msgindex);
        if (ret < 0)
        {
            abort();
        }
        strcat(temp_msg, temp_nmb);
        strcat(temp_msg, ")");
        strcat(temp_msg, "From ");
        strcat(temp_msg, x[IDsender].usr);
        strcat(temp_msg, " : ");
        strcat(temp_msg, message);
        printf("\nAm trimis : '%s'\n", temp_msg);
        strcpy(y[IDreceiver].inbox[msgindex], temp_msg); // adaugam mesajul local in structura de date inbox
        y[IDreceiver].senders[msgindex] = IDsender;// adaugam senderul mesajului local in structura de date inbox
        strcat(temp_msg, "\n");
        writeToInboxDB(IDreceiver, temp_msg); // scriem in baza de date a inboxurilor mesajul
        writeToArchiveSR(IDsender, IDreceiver, message); // scriem in arhive
        writeToArchiveRS(IDsender, IDreceiver, message);
    }
    else
    {
        writeToArchiveSR(IDsender, IDreceiver, message); // scriem in arhive
        writeToArchiveRS(IDsender, IDreceiver, message);
    }
}


//INBOX operations


void importInboxfromDB() // importam local inboxurile din baza de date
{
    char filename[20]; // aici cream path-ul
    char inboxindex[3]; // aici scriem ID-ul fiecarui user pentru a-i gasi inboxul
    char current_row[150]; // aici citim fiecare mesaj din inbox in parte
    for (int i = 1; i <= countID; i++) // iteram prin toti userii din sistem
    {
        bzero(inboxindex, sizeof(inboxindex));
        int ret = snprintf(inboxindex, 3, "%d", i);
        if (ret < 0)
        {
            abort();
        }
        bzero(filename, sizeof(filename));
        strcpy(filename, "inboxes/");
        strcat(filename, inboxindex);
        FILE* backup = fopen(filename, "a"); // daca nu exista, il cream
        if (backup == NULL)
        {
            perror("Eroare la deschiderea bazei de date");
            exit(1);
        }
        fclose(backup);
        FILE* fd = fopen(filename, "r");
        if (fd == NULL)
        {
            perror("Eroare la deschiderea bazei de date");
            exit(1);
        }
        bzero(current_row, sizeof(current_row));
        while (fgets(current_row, sizeof(current_row), fd) != NULL) // importam local fiecare inbox
        {
            current_row[strlen(current_row) - 1] = '\0';
            y[i].ID = i; // ID-ul este inregistrat
            y[i].msg_nmb++; // numarul de mesaje creste
            y[i].senders[y[i].msg_nmb] = current_row[1]; // inregistram fiecare sender (sintaxa din baza de date este (<ID sender>), deci este al doilea caracter din string)
            strcpy(y[i].inbox[y[i].msg_nmb], current_row); // inregistram local fiecare mesaj
        }
        fclose(fd);
    }
}
void showInboxes() // functie de debug, afisam fiecare inbox inregistrat local
{
    for (int j = 1; j <= countID; j++)
        if (y[j].msg_nmb == 0)
        {
            printf("\n'Niciun mesaj'\n");
        }
        else
        {
            for (int i = 1; i <= y[j].msg_nmb; i++)
            {
                printf("\n'%s'\n", y[j].inbox[i]);
            }
        }
}


//LOGIN operation


void addUser(char* username, char* password, char* firstname, char* lastname) // adaugam un user local
{
    countID = countID + 1; // creste numarul de useri din sistem
    x[countID].ID = countID;
    strcat(x[countID].usr, username);
    strcat(x[countID].pwd, password);
    strcat(x[countID].fn, firstname);
    strcat(x[countID].ln, lastname);
}
void showUsers() // afisam toti userii din sistem
{
    char temp_status[10];
    for (int i = 1; i <= countID; i++)
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
        printf("%d - %s - %s\n", x[i].ID, x[i].usr, temp_status);
    }
}
void addUsertoDB(char* filename, int ID) // adaugam userul in baza de date
{
    FILE* fd = fopen(filename, "a");
    if (fd == NULL)
    {
        perror("Eroare la deschiderea bazei de date");
        exit(1);
    }
    fprintf(fd, "%d;%s;%s;%s;%s\n", ID, x[ID].usr, x[ID].pwd, x[ID].fn, x[ID].ln); // cu delimitatorul ';', pentru a le putea importa cu usurinta
    fclose(fd);
}
void importUsersfromDB(char* filename) // importam local userii din baza de date
{
    FILE* backup = fopen(filename, "a"); // daca nu exista, cream baza de date
    if (backup == NULL)
    {
        perror("Eroare la deschiderea bazei de date");
        exit(1);
    }
    fclose(backup);
    FILE* fd = fopen(filename, "r");
    if (fd == NULL)
    {
        perror("Eroare la deschiderea bazei de date");
        exit(1);
    }
    char temp_user[50]; // retinem username
    char temp_pass[50]; // retinem parola
    char temp_fn[50]; // retinem prenumele
    char temp_ln[50]; // retinem numele
    char current_row[150];
    bzero(current_row, sizeof(current_row));
    bzero(temp_user, sizeof(temp_user));
    bzero(temp_pass, sizeof(temp_pass));
    bzero(temp_fn, sizeof(temp_fn));
    bzero(temp_ln, sizeof(temp_ln));
    char delim[] = ";";
    int info_ptr = 0;
    while (fgets(current_row, sizeof(current_row), fd) != NULL)
    {
        current_row[strlen(current_row) - 1] = '\0';
        char* ptr = strtok(current_row, delim);
        while (ptr != NULL && info_ptr <= 4)
        {
            switch (info_ptr)
            {
            case 0: // aici avem ID-ul, care se pune local automat prin functia addUser, deci nu il retinem
                ptr = strtok(NULL, delim);
                info_ptr++;
                break;
            case 1: // aici avem username-ul, il retinem
                strcpy(temp_user, ptr);
                ptr = strtok(NULL, delim);
                info_ptr++;
                break;
            case 2: // parola, o retinem
                strcpy(temp_pass, ptr);
                ptr = strtok(NULL, delim);
                info_ptr++;
                break;
            case 3: // prenumele, il retinem
                strcpy(temp_fn, ptr);
                ptr = strtok(NULL, delim);
                info_ptr++;
                break;
            case 4: // numele, il retinem
                strcpy(temp_ln, ptr);
                ptr = strtok(NULL, delim);
                info_ptr++;
                break;
            default: // eroare
                perror("Eroare la importarea bazei de date...");
                break;
            }
        }
        addUser(temp_user, temp_pass, temp_fn, temp_ln); // adaugam userul local
        bzero(current_row, sizeof(current_row));
        bzero(temp_user, sizeof(temp_user));
        bzero(temp_pass, sizeof(temp_pass));
        bzero(temp_fn, sizeof(temp_fn));
        bzero(temp_ln, sizeof(temp_ln));
        info_ptr = 0;
    }
    fclose(fd);
}


//functii UTILITARE pentru validare de login


int getIDbyName(char* user)
{
    for (int i = 1; i <= countID; i++)
    {
        if (strcmp(user, x[i].usr) == 0)
        {
            return x[i].ID;
        }
    }
    return -1;
}
int userCheck(char* user)
{
    for (int i = 1; i <= countID; i++)
    {
        if (strcmp(user, x[i].usr) == 0)
        {
            return x[i].ID;
        }
    }
    return -1;
}
void initStatuses() // initializam statusul OFFLINE la pornirea serverului
{
    for (int i = 1; i <= countID; i++)
    {
        status[i][0] = 0;
    }
}
void userLoggedIn(int ID) // schimbam statutul userului cu ID-ul dat in ONLINE
{
    if (status[ID][0] == 0)
        status[ID][0] = 1;
}
void userLoggedOut(int ID) // schimbam statutul userului cu ID-ul dat in OFFLINE
{
    if (status[ID][0] == 1)
        status[ID][0] = 0;
}
int passCheck(char* user, char* pwrd)
{
    if (strcmp(x[getIDbyName(user)].pwd, pwrd) == 0)
        return 1;
    return -1;
}
