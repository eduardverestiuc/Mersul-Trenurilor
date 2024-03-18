#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <stdint.h>
#include "sqlite3.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlexports.h>
#include <libxml/xmlversion.h>
#include <time.h>
#include <poll.h>
#define PORT 2908

extern int errno;

typedef struct thData
{
  int idThread;
  int cl;
} thData;

static void *treat(void *);
void raspunde(void *);

/*apelare pt baze de date sql*/

static int apelare_bd_verificare_existenta(void *data, int argc, char **argv, char **azColName)
{
  int i;

  for (i = 0; i < argc; i++)
  {
    {
      if (strcmp(data, argv[i]) == 0)
        strcpy(data, "Gasit!");
      /*
      if (argv[i])
        printf("%s = %s\n", azColName[i], argv[i]);
      else
        printf("%s = %s\n", azColName[i], "NULL");
        */
    }
  }
  return 0;
}

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
xmlDocPtr descriptor_fisier_xml;

typedef struct notificari_utilizatori
{
  char username[200];
  int id_tren_preferat[10];
  int numar_trenuri_preferate;
} notificari_utilizatori;
notificari_utilizatori preferinte[10];

typedef struct coada_intarzieri
{
  int indicativ_tren;
  char mesaj[100];

} coada_intarzieri;
coada_intarzieri coada_de_intarzieri[100];
int main()
{
  struct sockaddr_in server;
  struct sockaddr_in from;
  int sd;
  int pid;
  pthread_t th[100];
  int i = 0;

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("[server]Eroare la socket().\n");
    return errno;
  }

  int on = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  bzero(&server, sizeof(server));
  bzero(&from, sizeof(from));
  // reseteaza

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(PORT);

  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[server]Eroare la bind().\n");
    return errno;
  }

  if (listen(sd, 2) == -1)
  {
    perror("[server]Eroare la listen().\n");
    return errno;
  }

  while (1)
  {
    int client;
    thData *td;
    int length = sizeof(from);

    printf("[server]Se asteapta la portul: %d\n", PORT);
    fflush(stdout);

    if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
    {
      perror("[server]Eroare la accept().\n");
      continue;
    }

    td = (struct thData *)malloc(sizeof(struct thData));
    td->idThread = i++;
    td->cl = client;

    pthread_create(&th[i], NULL, &treat, td);
  }
};
static void *treat(void *arg)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);
  printf("[thread]- %d - Asteptam comanda...\n", tdL.idThread);
  fflush(stdout);
  pthread_detach(pthread_self());
  raspunde((struct thData *)arg);
  close((intptr_t)arg);
  return (NULL);
};

void raspunde(void *arg)
{
  int logat = 0;
  int intarziere_vazuta = 0;
  while (1)
  {
    int i = 0;
    char comanda_primita[200] = {0};
    int dimensiune_comanda_primita;
    char raspuns_comanda[200] = {0};
    int dimensiune_raspuns_comanda;
    char localitate1[200] = {0};
    char localitate2[200] = {0};
    char nume_utilizator[200];
    char statie[200] = {0};
    char numar_minute[10] = {0};
    char id_modificare_primit[10] = {0};
    char id_preferat_primit[10] = {0};
    struct thData tdL;
    tdL = *((struct thData *)arg);

    int timeout = 2000;
    struct pollfd poll_fds[1];
    poll_fds[0].fd = tdL.cl;
    poll_fds[0].events = POLLIN;
    int rezultat = poll(poll_fds, 1, timeout);
    if (rezultat == 0)
    {
      // printf("Pune the Ketchup song\n");
      pthread_mutex_lock(&mlock);
      if (strlen(coada_de_intarzieri[intarziere_vazuta].mesaj) > 0)
      {
        for (i = 0; i <= 9; i++)
        {
          if (strcmp(preferinte[i].username, nume_utilizator) == 0)
          {
            for (int j = 0; j < preferinte[i].numar_trenuri_preferate; j++)
              if (coada_de_intarzieri[intarziere_vazuta].indicativ_tren == preferinte[i].id_tren_preferat[j])
              {
                char trimitere_notificari[100];
                int trimitere_notificari_dimensiune;
                strcpy(trimitere_notificari, coada_de_intarzieri[intarziere_vazuta].mesaj);
                trimitere_notificari_dimensiune = strlen(trimitere_notificari);
                write(tdL.cl, &trimitere_notificari_dimensiune, sizeof(int));
                write(tdL.cl, trimitere_notificari, trimitere_notificari_dimensiune);
              }
          }
        }
        intarziere_vazuta++;
      }
      pthread_mutex_unlock(&mlock);
    }
    else
    {
      if (poll_fds[0].revents & POLLIN)
      {
        read(tdL.cl, &dimensiune_comanda_primita, sizeof(int));
        read(tdL.cl, comanda_primita, dimensiune_comanda_primita);
      }
    }
    // printf("%s\n", comanda_primita);
    if (strncmp(comanda_primita, "login : ", 8) == 0)
    {
      printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
      char mesaj[100] = {0};
      int mesaj_dimensiune;
      strcpy(mesaj, "Am primit comanda - login -");
      mesaj_dimensiune = strlen(mesaj);
      write(tdL.cl, &mesaj_dimensiune, sizeof(int));
      write(tdL.cl, mesaj, mesaj_dimensiune);
      if (logat == 0)
      {
        strcpy(nume_utilizator, comanda_primita + 8);
        // printf("%s\n", nume_utilizator);
        sqlite3 *db;
        char *mesaj_eroare = 0;
        int desc_bd;
        char *sql;
        char date_trimise_spre_verif[200];
        strcpy(date_trimise_spre_verif, nume_utilizator);
        desc_bd = sqlite3_open("baza_date_utilizatori.db", &db);
        if (desc_bd)
        {
          printf("[Thread %d]Baza de date nu a putut fi deschisa! \n", tdL.idThread);
        }
        else
        {
          printf("[Thread %d]Baza de date deschisa cu succes! \n", tdL.idThread);
        }
        sql = "SELECT nume_utilizator from utilizatori";
        desc_bd = sqlite3_exec(db, sql, apelare_bd_verificare_existenta, (void *)date_trimise_spre_verif, &mesaj_eroare);
        if (desc_bd != SQLITE_OK)
        {
          fprintf(stderr, "Eroare Sql: %s\n", mesaj_eroare);
          sqlite3_free(mesaj_eroare);
        }
        else
        {
          printf("[Thread %d]Verificare nume utilizator efectuata! \n", tdL.idThread);
        }
        if (strcmp(date_trimise_spre_verif, "Gasit!") == 0)
        {
          strcpy(raspuns_comanda, "Numele de utilizator este corect!");
          dimensiune_raspuns_comanda = strlen(raspuns_comanda);
          printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, raspuns_comanda);
          write(tdL.cl, &dimensiune_raspuns_comanda, sizeof(int));
          write(tdL.cl, raspuns_comanda, dimensiune_raspuns_comanda);
          printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
          printf("\n");

          char parola_primita[200] = {0};
          int dimensiune_parola_primita;
          read(tdL.cl, &dimensiune_parola_primita, sizeof(int));
          read(tdL.cl, parola_primita, dimensiune_parola_primita);
          // printf("%i\n", dimensiune_parola_primita);
          printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, parola_primita);
          strcpy(date_trimise_spre_verif, parola_primita);
          sql = "SELECT parola from utilizatori";
          desc_bd = sqlite3_exec(db, sql, apelare_bd_verificare_existenta, (void *)date_trimise_spre_verif, &mesaj_eroare);
          if (desc_bd != SQLITE_OK)
          {
            fprintf(stderr, "Eroare Sql: %s\n", mesaj_eroare);
            sqlite3_free(mesaj_eroare);
          }
          else
          {
            printf("[Thread %d]Verificare parola efectuata! \n", tdL.idThread);
          }
          if (strcmp(date_trimise_spre_verif, "Gasit!") == 0)
          {
            strcpy(raspuns_comanda, "Utilizator conectat cu succes!");
            logat = 1;
          }
          else
          {
            strcpy(raspuns_comanda, "Parola incorecta!");
          }
          dimensiune_raspuns_comanda = strlen(raspuns_comanda);
          printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, raspuns_comanda);
          write(tdL.cl, &dimensiune_raspuns_comanda, sizeof(int));
          write(tdL.cl, raspuns_comanda, dimensiune_raspuns_comanda);
          printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
          printf("\n");
        }
        else
        {
          strcpy(raspuns_comanda, "Utilizator inexistent! Inregistrati-va!");
          dimensiune_raspuns_comanda = strlen(raspuns_comanda);
          printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, raspuns_comanda);
          write(tdL.cl, &dimensiune_raspuns_comanda, sizeof(int));
          write(tdL.cl, raspuns_comanda, dimensiune_raspuns_comanda);
          printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
          printf("\n");
        }
        sqlite3_close(db);
      }
      else
      {
        strcpy(raspuns_comanda, "Alt utilizator deja conectat!");
        dimensiune_raspuns_comanda = strlen(raspuns_comanda);
        printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, raspuns_comanda);
        write(tdL.cl, &dimensiune_raspuns_comanda, sizeof(int));
        write(tdL.cl, raspuns_comanda, dimensiune_raspuns_comanda);
        printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
        printf("\n");
      }
    }

    if (strncmp(comanda_primita, "sign up : ", 10) == 0)
    {
      printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
      char mesaj[100] = {0};
      int mesaj_dimensiune;
      strcpy(mesaj, "Am primit comanda - sign up : username -");
      mesaj_dimensiune = strlen(mesaj);
      write(tdL.cl, &mesaj_dimensiune, sizeof(int));
      write(tdL.cl, mesaj, mesaj_dimensiune);
      if (logat == 0)
      {
        char nume_utilizator[200];
        strcpy(nume_utilizator, comanda_primita + 10);
        sqlite3 *db;
        char *mesaj_eroare = 0;
        int desc_bd;
        char *sql;
        char date_trimise_spre_verif[200];
        strcpy(date_trimise_spre_verif, nume_utilizator);
        desc_bd = sqlite3_open("baza_date_utilizatori.db", &db);
        if (desc_bd)
        {
          printf("[Thread %d]Baza de date nu a putut fi deschisa! \n", tdL.idThread);
        }
        else
        {
          printf("[Thread %d]Baza de date deschisa cu succes! \n", tdL.idThread);
        }
        sql = "SELECT nume_utilizator from utilizatori";
        desc_bd = sqlite3_exec(db, sql, apelare_bd_verificare_existenta, (void *)date_trimise_spre_verif, &mesaj_eroare);
        if (desc_bd != SQLITE_OK)
        {
          fprintf(stderr, "Eroare Sql: %s\n", mesaj_eroare);
          sqlite3_free(mesaj_eroare);
        }
        else
        {
          printf("[Thread %d]Verificare nume utilizator efectuata! \n", tdL.idThread);
        }
        if (strcmp(date_trimise_spre_verif, "Gasit!") == 0)
        {
          strcpy(raspuns_comanda, "Numele de utilizator deja existent!");
          dimensiune_raspuns_comanda = strlen(raspuns_comanda);
          printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, raspuns_comanda);
          write(tdL.cl, &dimensiune_raspuns_comanda, sizeof(int));
          write(tdL.cl, raspuns_comanda, dimensiune_raspuns_comanda);
          printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
          printf("\n");
        }
        else
        {
          strcpy(raspuns_comanda, "Nume de utilizator inregistrat cu succes!");
          dimensiune_raspuns_comanda = strlen(raspuns_comanda);
          printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, raspuns_comanda);
          write(tdL.cl, &dimensiune_raspuns_comanda, sizeof(int));
          write(tdL.cl, raspuns_comanda, dimensiune_raspuns_comanda);
          printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
          printf("\n");
          char parola_utilizator[200] = {0};
          int dimensiune_parola_utilizator;
          read(tdL.cl, &dimensiune_parola_utilizator, sizeof(int));
          read(tdL.cl, parola_utilizator, dimensiune_parola_utilizator);
          printf("%s\n", parola_utilizator);
          char sql_buffer[600];
          sprintf(sql_buffer, "INSERT INTO utilizatori (nume_utilizator, parola) VALUES('%s','%s')", nume_utilizator, parola_utilizator);
          sql = sql_buffer;
          printf("%s\n", sql);
          desc_bd = sqlite3_exec(db, sql, apelare_bd_verificare_existenta, (void *)date_trimise_spre_verif, &mesaj_eroare);
          if (desc_bd != SQLITE_OK)
          {
            fprintf(stderr, "Eroare Sql: %s\n", mesaj_eroare);
            sqlite3_free(mesaj_eroare);
          }
          else
          {
            printf("[Thread %d]Inregistrare utilizator efectuata! \n", tdL.idThread);
          }
          strcpy(raspuns_comanda, "Utilizator inregistrat cu succes!");
          dimensiune_raspuns_comanda = strlen(raspuns_comanda);
          write(tdL.cl, &dimensiune_raspuns_comanda, sizeof(int));
          write(tdL.cl, raspuns_comanda, dimensiune_raspuns_comanda);
          printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
          printf("\n");
        }
      }
      else
      {
        strcpy(raspuns_comanda, "Alt utilizator deja conectat!");
        dimensiune_raspuns_comanda = strlen(raspuns_comanda);
        printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, raspuns_comanda);
        write(tdL.cl, &dimensiune_raspuns_comanda, sizeof(int));
        write(tdL.cl, raspuns_comanda, dimensiune_raspuns_comanda);
        printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
        printf("\n");
      }
    }

    if (strcmp(comanda_primita, "logout") == 0)
    {
      printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
      char mesaj[100] = {0};
      int mesaj_dimensiune;
      strcpy(mesaj, "Am primit comanda - logout -");
      mesaj_dimensiune = strlen(mesaj);
      write(tdL.cl, &mesaj_dimensiune, sizeof(int));
      write(tdL.cl, mesaj, mesaj_dimensiune);
      if (logat != 0)
      {
        logat = 0;
        strcpy(raspuns_comanda, "Utilizator deconectat cu succes!");
      }
      else
      {
        strcpy(raspuns_comanda, "Logout incorect, niciun utilizator conectat!");
      }
      // printf("%s\n",raspuns_comanda);
      printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, raspuns_comanda);
      int dimensiune_raspuns_comanda = 0;
      dimensiune_raspuns_comanda = strlen(raspuns_comanda);
      // printf("%i\n", dimensiune_raspuns_comanda);
      write(tdL.cl, &dimensiune_raspuns_comanda, sizeof(int));
      write(tdL.cl, raspuns_comanda, dimensiune_raspuns_comanda);
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
      printf("\n");
    }

    if (strcmp(comanda_primita, "quit") == 0)
    {
      if (logat != 0)
      {
        printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
        char mesaj[100] = {0};
        int mesaj_dimensiune;
        strcpy(mesaj, "Am primit comanda - quit -");
        mesaj_dimensiune = strlen(mesaj);
        write(tdL.cl, &mesaj_dimensiune, sizeof(int));
        write(tdL.cl, mesaj, mesaj_dimensiune);
        strcpy(raspuns_comanda, "Deconectati-va mai intai!");
        int dimensiune_raspuns_comanda = 0;
        dimensiune_raspuns_comanda = strlen(raspuns_comanda);
        write(tdL.cl, &dimensiune_raspuns_comanda, sizeof(int));
        write(tdL.cl, raspuns_comanda, dimensiune_raspuns_comanda);
        printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
        printf("\n");
      }
      else
      {
        printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
        char mesaj[100] = {0};
        int mesaj_dimensiune;
        strcpy(mesaj, "Am primit comanda - quit -");
        mesaj_dimensiune = strlen(mesaj);
        write(tdL.cl, &mesaj_dimensiune, sizeof(int));
        write(tdL.cl, mesaj, mesaj_dimensiune);
        printf("[Thread %d]Thread %d a fost inchis cu succes! \n", tdL.idThread, tdL.idThread);
        return;
      }
    }

    if (strcmp(comanda_primita, "lista trenuri astazi") == 0)
    {
      printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
      char mesaj[100] = {0};
      int mesaj_dimensiune;
      char informatii_trenuri[2200] = {0};
      int informatii_trenuri_dimensiune;
      strcpy(mesaj, "Am primit comanda - lista trenuri astazi -");
      mesaj_dimensiune = strlen(mesaj);
      write(tdL.cl, &mesaj_dimensiune, sizeof(int));
      write(tdL.cl, mesaj, mesaj_dimensiune);
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
      if (logat != 0)
      {
        xmlNodePtr trenuri, nodcurent_tren, nodcurent_statie;
        xmlChar *id_tren, *nume_statie, *ora_plecare, *ora_sosire, *intarziere;
        pthread_mutex_lock(&mlock);
        descriptor_fisier_xml = xmlReadFile("trenuri.xml", NULL, 0);
        trenuri = xmlDocGetRootElement(descriptor_fisier_xml);
        for (nodcurent_tren = trenuri->children; nodcurent_tren; nodcurent_tren = nodcurent_tren->next)
        {
          if (nodcurent_tren->type == XML_ELEMENT_NODE)
          {
            if (xmlStrcmp(nodcurent_tren->name, (xmlChar *)"tren") == 0)
            {
              id_tren = xmlGetProp(nodcurent_tren, (xmlChar *)"id");
              sprintf(informatii_trenuri + strlen(informatii_trenuri), "tren IR-%s \n", (char *)id_tren);
              for (nodcurent_statie = nodcurent_tren->children; nodcurent_statie; nodcurent_statie = nodcurent_statie->next)
              {
                if (nodcurent_statie->type == XML_ELEMENT_NODE)
                {
                  if (xmlStrcmp(nodcurent_statie->name, (xmlChar *)"statie") == 0)
                  {
                    nume_statie = xmlGetProp(nodcurent_statie, (xmlChar *)"nume");
                    sprintf(informatii_trenuri + strlen(informatii_trenuri), "Statia: %s \n", (char *)nume_statie);
                    ora_sosire = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_sosire");
                    if (ora_sosire != NULL)
                      sprintf(informatii_trenuri + strlen(informatii_trenuri), "Ora sosire: %s \n", (char *)ora_sosire);
                    ora_plecare = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_plecare");
                    if (ora_plecare != NULL)
                      sprintf(informatii_trenuri + strlen(informatii_trenuri), "Ora plecare: %s \n", (char *)ora_plecare);
                    intarziere = xmlGetProp(nodcurent_statie, (xmlChar *)"intarziere");
                    char c = '-';
                    if ((char)intarziere[0] == c)
                      sprintf(informatii_trenuri + strlen(informatii_trenuri), "Mai devreme la aceasta statie: %s \n", (char *)(intarziere + 1));
                    else
                      sprintf(informatii_trenuri + strlen(informatii_trenuri), "Intarzierea la aceasta statie: %s \n", (char *)intarziere);
                  }
                }
              }
              sprintf(informatii_trenuri + strlen(informatii_trenuri), "\n");
            }
          }
        }
        informatii_trenuri_dimensiune = strlen(informatii_trenuri);
        write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
        write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
        printf("%s\n", informatii_trenuri);
        printf("[Thread %d]Mesajul a fost transmis cu succes! \n", tdL.idThread);
        xmlFree(id_tren);
        xmlFree(ora_plecare);
        xmlFree(ora_sosire);
        xmlFree(nume_statie);
        xmlFree(intarziere);
        xmlFree(descriptor_fisier_xml);
        pthread_mutex_unlock(&mlock);
      }
      else
      {
        strcpy(informatii_trenuri, "Niciun utilizator conectat!\n");
        informatii_trenuri_dimensiune = strlen(informatii_trenuri);
        printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, informatii_trenuri);
        write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
        write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
        printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
        printf("\n");
      }
    }

    if (strncmp(comanda_primita, "tren IR-", 8) == 0)
    {
      printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
      char mesaj[100] = {0};
      int mesaj_dimensiune;
      char informatii_trenuri[1200] = {0};
      int informatii_trenuri_dimensiune;
      strcpy(mesaj, "Am primit comanda - tren IR-id_tren -");
      mesaj_dimensiune = strlen(mesaj);
      write(tdL.cl, &mesaj_dimensiune, sizeof(int));
      write(tdL.cl, mesaj, mesaj_dimensiune);
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
      if (logat != 0)
      {
        char id_tren_primit[10];
        strcpy(id_tren_primit, comanda_primita + 8);
        xmlNodePtr trenuri, nodcurent_tren, nodcurent_statie;
        xmlChar *id_tren, *nume_statie, *ora_plecare, *ora_sosire, *intarziere;
        pthread_mutex_lock(&mlock);
        descriptor_fisier_xml = xmlReadFile("trenuri.xml", NULL, 0);
        trenuri = xmlDocGetRootElement(descriptor_fisier_xml);
        for (nodcurent_tren = trenuri->children; nodcurent_tren; nodcurent_tren = nodcurent_tren->next)
        {
          if (nodcurent_tren->type == XML_ELEMENT_NODE)
          {
            if (xmlStrcmp(nodcurent_tren->name, (xmlChar *)"tren") == 0)
            {
              id_tren = xmlGetProp(nodcurent_tren, (xmlChar *)"id");
              if (xmlStrcmp(id_tren, (xmlChar *)id_tren_primit) == 0)
              {
                sprintf(informatii_trenuri + strlen(informatii_trenuri), "\ntren IR-%s \n", (char *)id_tren);
                for (nodcurent_statie = nodcurent_tren->children; nodcurent_statie; nodcurent_statie = nodcurent_statie->next)
                {
                  if (nodcurent_statie->type == XML_ELEMENT_NODE)
                  {
                    if (xmlStrcmp(nodcurent_statie->name, (xmlChar *)"statie") == 0)
                    {
                      nume_statie = xmlGetProp(nodcurent_statie, (xmlChar *)"nume");
                      sprintf(informatii_trenuri + strlen(informatii_trenuri), "Statia: %s \n", (char *)nume_statie);
                      ora_sosire = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_sosire");
                      if (ora_sosire != NULL)
                        sprintf(informatii_trenuri + strlen(informatii_trenuri), "Ora sosire: %s \n", (char *)ora_sosire);
                      ora_plecare = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_plecare");
                      if (ora_plecare != NULL)
                        sprintf(informatii_trenuri + strlen(informatii_trenuri), "Ora plecare: %s \n", (char *)ora_plecare);
                      intarziere = xmlGetProp(nodcurent_statie, (xmlChar *)"intarziere");
                      char c = '-';
                      if ((char)intarziere[0] == c)
                        sprintf(informatii_trenuri + strlen(informatii_trenuri), "Mai devreme la aceasta statie: %s \n", (char *)(intarziere + 1));
                      else
                        sprintf(informatii_trenuri + strlen(informatii_trenuri), "Intarzierea la aceasta statie: %s \n", (char *)intarziere);
                    }
                  }
                }
              }
            }
          }
        }
        informatii_trenuri_dimensiune = strlen(informatii_trenuri);
        if (informatii_trenuri_dimensiune == 0)
        {
          strcpy(informatii_trenuri, "Nu am gasit trenul cu acest indicativ!");
          informatii_trenuri_dimensiune = strlen(informatii_trenuri);
        }
        write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
        write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
        printf("%s\n", informatii_trenuri);
        printf("[Thread %d]Mesajul a fost transmis cu succes! \n", tdL.idThread);
        xmlFree(id_tren);
        xmlFree(ora_plecare);
        xmlFree(ora_sosire);
        xmlFree(nume_statie);
        xmlFree(intarziere);
        xmlFree(descriptor_fisier_xml);
        pthread_mutex_unlock(&mlock);
      }
      else
      {
        {
          strcpy(informatii_trenuri, "Niciun utilizator conectat!");
          informatii_trenuri_dimensiune = strlen(informatii_trenuri);
          printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, informatii_trenuri);
          write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
          write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
          printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
          printf("\n");
        }
      }
    }

    if (strcmp(comanda_primita, "plecari trenuri urmatoarea ora") == 0)
    {
      printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
      char mesaj[100] = {0};
      int mesaj_dimensiune;
      char informatii_trenuri[1200] = {0};
      int informatii_trenuri_dimensiune;
      strcpy(mesaj, "Am primit comanda - plecari trenuri urmatoarea ora -");
      mesaj_dimensiune = strlen(mesaj);
      write(tdL.cl, &mesaj_dimensiune, sizeof(int));
      write(tdL.cl, mesaj, mesaj_dimensiune);
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
      if (logat != 0)
      {
        int traseu_marcat = 0;
        char id_tren_primit[10];
        strcpy(id_tren_primit, comanda_primita + 8);
        xmlNodePtr trenuri, nodcurent_tren, nodcurent_statie;
        xmlChar *id_tren, *nume_statie, *ora_plecare, *ora_sosire, *intarziere;
        pthread_mutex_lock(&mlock);
        time_t timp;
        struct tm *info_timp;
        time(&timp);
        info_timp = localtime(&timp);
        // printf("%d\n",info_timp->tm_hour);
        // printf("%d\n",info_timp->tm_min);
        descriptor_fisier_xml = xmlReadFile("trenuri.xml", NULL, 0);
        trenuri = xmlDocGetRootElement(descriptor_fisier_xml);
        for (nodcurent_tren = trenuri->children; nodcurent_tren; nodcurent_tren = nodcurent_tren->next)
        {
          if (nodcurent_tren->type == XML_ELEMENT_NODE)
          {
            if (xmlStrcmp(nodcurent_tren->name, (xmlChar *)"tren") == 0)
            {
              id_tren = xmlGetProp(nodcurent_tren, (xmlChar *)"id");
              for (nodcurent_statie = nodcurent_tren->children; nodcurent_statie; nodcurent_statie = nodcurent_statie->next)
              {
                if (nodcurent_statie->type == XML_ELEMENT_NODE)
                {
                  if (xmlStrcmp(nodcurent_statie->name, (xmlChar *)"statie") == 0)
                  {
                    ora_plecare = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_plecare");
                    if (traseu_marcat == 1)
                    {
                      nume_statie = xmlGetProp(nodcurent_statie, (xmlChar *)"nume");
                      sprintf(informatii_trenuri + strlen(informatii_trenuri), "%s ", (char *)nume_statie);
                    }
                    if (ora_plecare != NULL)
                    {
                      int ora_plecare_in_int = atoi((char *)ora_plecare);
                      int minute_plecare_in_int;
                      if (ora_plecare_in_int >= 10)
                      {
                        minute_plecare_in_int = atoi((char *)ora_plecare + 3);
                      }
                      else
                      {
                        minute_plecare_in_int = atoi((char *)ora_plecare + 2);
                      }
                      // printf("%d\n", minute_plecare_in_int);
                      if (info_timp->tm_hour + 1 == 24 && ora_plecare_in_int == 0)
                      {
                        ora_plecare_in_int = 24;
                      }
                      if (((ora_plecare_in_int == info_timp->tm_hour + 1) && (minute_plecare_in_int <= info_timp->tm_min)) || ((ora_plecare_in_int == info_timp->tm_hour) && (minute_plecare_in_int >= info_timp->tm_min)))
                      {
                        sprintf(informatii_trenuri + strlen(informatii_trenuri), "\ntren IR-%s: \n", (char *)id_tren);
                        nume_statie = xmlGetProp(nodcurent_statie, (xmlChar *)"nume");
                        sprintf(informatii_trenuri + strlen(informatii_trenuri), "Plecare din statia: %s \n", (char *)nume_statie);
                        sprintf(informatii_trenuri + strlen(informatii_trenuri), "La ora: %s \nIn directia: ", (char *)ora_plecare);
                        traseu_marcat = 1;
                      }
                    }
                  }
                }
              }
              if (traseu_marcat == 1)
              {
                sprintf(informatii_trenuri + strlen(informatii_trenuri), "\n");
                traseu_marcat = 0;
              }
            }
          }
        }
        informatii_trenuri_dimensiune = strlen(informatii_trenuri);
        if (informatii_trenuri_dimensiune == 0)
        {
          strcpy(informatii_trenuri, "Nu sunt plecari inregistrate in urmatoarea ora!");
          informatii_trenuri_dimensiune = strlen(informatii_trenuri);
        }
        write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
        write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
        printf("%s\n", informatii_trenuri);
        printf("[Thread %d]Mesajul a fost transmis cu succes! \n", tdL.idThread);
        xmlFree(id_tren);
        xmlFree(ora_plecare);
        xmlFree(ora_sosire);
        xmlFree(nume_statie);
        xmlFree(intarziere);
        xmlFree(descriptor_fisier_xml);
        pthread_mutex_unlock(&mlock);
      }
      else
      {
        {
          strcpy(informatii_trenuri, "Niciun utilizator conectat!");
          informatii_trenuri_dimensiune = strlen(informatii_trenuri);
          printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, informatii_trenuri);
          write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
          write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
          printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
          printf("\n");
        }
      }
    }

    if (strcmp(comanda_primita, "sosiri trenuri urmatoarea ora") == 0)
    {
      printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
      char mesaj[100] = {0};
      int mesaj_dimensiune;
      char informatii_trenuri[1200] = {0};
      int informatii_trenuri_dimensiune;
      strcpy(mesaj, "Am primit comanda - sosiri trenuri urmatoarea ora -");
      mesaj_dimensiune = strlen(mesaj);
      write(tdL.cl, &mesaj_dimensiune, sizeof(int));
      write(tdL.cl, mesaj, mesaj_dimensiune);
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
      if (logat != 0)
      {
        int traseu_marcat = 0;
        char id_tren_primit[10];
        strcpy(id_tren_primit, comanda_primita + 8);
        xmlNodePtr trenuri, nodcurent_tren, nodcurent_statie;
        xmlChar *id_tren, *nume_statie, *ora_plecare, *ora_sosire, *intarziere;
        pthread_mutex_lock(&mlock);
        time_t timp;
        struct tm *info_timp;
        time(&timp);
        info_timp = localtime(&timp);
        // printf("%d\n",info_timp->tm_hour);
        // printf("%d\n",info_timp->tm_min);
        descriptor_fisier_xml = xmlReadFile("trenuri.xml", NULL, 0);
        trenuri = xmlDocGetRootElement(descriptor_fisier_xml);
        for (nodcurent_tren = trenuri->children; nodcurent_tren; nodcurent_tren = nodcurent_tren->next)
        {
          if (nodcurent_tren->type == XML_ELEMENT_NODE)
          {
            if (xmlStrcmp(nodcurent_tren->name, (xmlChar *)"tren") == 0)
            {
              id_tren = xmlGetProp(nodcurent_tren, (xmlChar *)"id");
              for (nodcurent_statie = nodcurent_tren->children; nodcurent_statie; nodcurent_statie = nodcurent_statie->next)
              {
                if (nodcurent_statie->type == XML_ELEMENT_NODE)
                {
                  if (xmlStrcmp(nodcurent_statie->name, (xmlChar *)"statie") == 0)
                  {
                    ora_sosire = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_sosire");
                    if (traseu_marcat == 1)
                    {
                      nume_statie = xmlGetProp(nodcurent_statie, (xmlChar *)"nume");
                      sprintf(informatii_trenuri + strlen(informatii_trenuri), "%s ", (char *)nume_statie);
                    }
                    if (ora_sosire != NULL)
                    {
                      int ora_sosire_in_int = atoi((char *)ora_sosire);
                      // printf("%d\n", ora_plecare_in_int);
                      int minute_sosire_in_int;
                      if (ora_sosire_in_int >= 10)
                      {
                        minute_sosire_in_int = atoi((char *)ora_sosire + 3);
                      }
                      else
                      {
                        minute_sosire_in_int = atoi((char *)ora_sosire + 2);
                      }
                      if (info_timp->tm_hour + 1 == 24 && ora_sosire_in_int == 0)
                      {
                        ora_sosire_in_int = 24;
                      }
                      if (((ora_sosire_in_int == info_timp->tm_hour + 1) && (minute_sosire_in_int <= info_timp->tm_min)) || ((ora_sosire_in_int == info_timp->tm_hour) && (minute_sosire_in_int >= info_timp->tm_min)))
                      {
                        sprintf(informatii_trenuri + strlen(informatii_trenuri), "\ntren IR-%s: \n", (char *)id_tren);
                        nume_statie = xmlGetProp(nodcurent_statie, (xmlChar *)"nume");
                        sprintf(informatii_trenuri + strlen(informatii_trenuri), "Sosire in statia: %s\n", (char *)nume_statie);
                        sprintf(informatii_trenuri + strlen(informatii_trenuri), "La ora: %s", (char *)ora_sosire);
                        ora_plecare = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_plecare");
                        if (ora_plecare != NULL)
                          sprintf(informatii_trenuri + strlen(informatii_trenuri), "\nPlecare in directia: ");
                        traseu_marcat = 1;
                      }
                    }
                  }
                }
              }
              if (traseu_marcat == 1)
              {
                sprintf(informatii_trenuri + strlen(informatii_trenuri), "\n");
                traseu_marcat = 0;
              }
            }
          }
        }
        informatii_trenuri_dimensiune = strlen(informatii_trenuri);
        if (informatii_trenuri_dimensiune == 0)
        {
          strcpy(informatii_trenuri, "Nu sunt sosiri inregistrate in urmatoarea ora!");
          informatii_trenuri_dimensiune = strlen(informatii_trenuri);
        }
        write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
        write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
        printf("%s\n", informatii_trenuri);
        printf("[Thread %d]Mesajul a fost transmis cu succes! \n", tdL.idThread);
        xmlFree(id_tren);
        xmlFree(ora_plecare);
        xmlFree(ora_sosire);
        xmlFree(nume_statie);
        xmlFree(intarziere);
        xmlFree(descriptor_fisier_xml);
        pthread_mutex_unlock(&mlock);
      }
      else
      {
        {
          strcpy(informatii_trenuri, "Niciun utilizator conectat!");
          informatii_trenuri_dimensiune = strlen(informatii_trenuri);
          printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, informatii_trenuri);
          write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
          write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
          printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
          printf("\n");
        }
      }
    }

    if (sscanf(comanda_primita, "notificari tren IR-%s", id_preferat_primit) == 1)
    {
      printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
      char mesaj[100] = {0};
      int mesaj_dimensiune;
      char informatii_trenuri[1200] = {0};
      int informatii_trenuri_dimensiune;
      strcpy(mesaj, "Am primit comanda - notificari tren IR-id_tren -");
      mesaj_dimensiune = strlen(mesaj);
      write(tdL.cl, &mesaj_dimensiune, sizeof(int));
      write(tdL.cl, mesaj, mesaj_dimensiune);
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
      int id_primit;
      if (logat != 0)
      {
        pthread_mutex_lock(&mlock);
        id_primit = atoi(id_preferat_primit);
        int exista = -1;
        for (int i = 0; i <= 9; i++)
        {
          if (strcmp(preferinte[i].username, nume_utilizator) == 0)
          {
            exista = i;
            preferinte[i].id_tren_preferat[preferinte[i].numar_trenuri_preferate] = id_primit;
            preferinte[i].numar_trenuri_preferate++;
            // printf("%s\n", preferinte[i].username);
            // printf("%d\n", preferinte[i].id_tren_preferat[preferinte[i].numar_trenuri_preferate-1]);
            // printf("%d\n", preferinte[i].numar_trenuri_preferate);
          }
        }
        if (exista == -1)
        {
          for (int i = 0; i <= 9 && exista == -1; i++)
          {
            if (strlen(preferinte[i].username) == 0)
            {
              strcpy(preferinte[i].username, nume_utilizator);
              preferinte[i].id_tren_preferat[preferinte[i].numar_trenuri_preferate] = id_primit;
              preferinte[i].numar_trenuri_preferate++;
              // printf("%s\n", preferinte[i].username);
              // printf("%d\n", preferinte[i].id_tren_preferat[preferinte[i].numar_trenuri_preferate-1]);
              // printf("%d\n", preferinte[i].numar_trenuri_preferate);
              // printf("%d\n", id_primit);
              exista = i;
            }
          }
        }
        strcpy(informatii_trenuri, "Preferinta salvata cu succes!");
        informatii_trenuri_dimensiune = strlen(informatii_trenuri);
        printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, informatii_trenuri);
        write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
        write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
        printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
        printf("\n");
        pthread_mutex_unlock(&mlock);
      }
      else
      {
        {
          strcpy(informatii_trenuri, "Niciun utilizator conectat!");
          informatii_trenuri_dimensiune = strlen(informatii_trenuri);
          printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, informatii_trenuri);
          write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
          write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
          printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
          printf("\n");
        }
      }
    }

    if (sscanf(comanda_primita, "intarziere tren IR-%s %s min %s", id_modificare_primit, numar_minute, statie) == 3)
    {
      printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
      char mesaj[100] = {0};
      int mesaj_dimensiune;
      char informatii_trenuri[100] = {0};
      int informatii_trenuri_dimensiune;
      int am_trecut = 0;
      int numar_minute_int = atoi(numar_minute);
      int intarziere_actuala = 0;
      strcat(numar_minute, "min");
      strcpy(mesaj, "Am primit comanda - intarziere tren IR-id_tren x min statie -");
      mesaj_dimensiune = strlen(mesaj);
      write(tdL.cl, &mesaj_dimensiune, sizeof(int));
      write(tdL.cl, mesaj, mesaj_dimensiune);
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
      // printf("%s\n", id_modificare_primit);
      // printf("%s\n", numar_minute);
      // printf("%s\n", statie);
      // printf("%d\n", numar_minute_int);
      if (logat != 0)
      {
        xmlNodePtr trenuri, nodcurent_tren, nodcurent_statie;
        xmlChar *id_tren, *nume_statie, *ora_plecare, *ora_sosire, *intarziere;
        pthread_mutex_lock(&mlock);
        descriptor_fisier_xml = xmlReadFile("trenuri.xml", NULL, 0);
        trenuri = xmlDocGetRootElement(descriptor_fisier_xml);
        for (nodcurent_tren = trenuri->children; nodcurent_tren; nodcurent_tren = nodcurent_tren->next)
        {
          if (nodcurent_tren->type == XML_ELEMENT_NODE)
          {
            if (xmlStrcmp(nodcurent_tren->name, (xmlChar *)"tren") == 0)
            {
              id_tren = xmlGetProp(nodcurent_tren, (xmlChar *)"id");
              if (xmlStrcmp(id_tren, (xmlChar *)id_modificare_primit) == 0)
              {
                for (nodcurent_statie = nodcurent_tren->children; nodcurent_statie; nodcurent_statie = nodcurent_statie->next)
                {
                  if (nodcurent_statie->type == XML_ELEMENT_NODE)
                  {
                    if (xmlStrcmp(nodcurent_statie->name, (xmlChar *)"statie") == 0)
                    {
                      nume_statie = xmlGetProp(nodcurent_statie, (xmlChar *)"nume");
                      if (am_trecut == 1)
                      {
                        intarziere = xmlGetProp(nodcurent_statie, (xmlChar *)"intarziere");
                        intarziere_actuala = atoi((char *)intarziere);
                        int intarziere_totala = numar_minute_int + intarziere_actuala;
                        sprintf(numar_minute, "%dmin", intarziere_totala);
                        xmlSetProp(nodcurent_statie, (xmlChar *)"intarziere", (xmlChar *)numar_minute);
                        ora_plecare = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_plecare");
                        if (ora_plecare != NULL)
                        {
                          int ora_plecare_in_int = atoi((char *)ora_plecare);
                          int minute_plecare_in_int;
                          if (ora_plecare_in_int >= 10)
                          {
                            minute_plecare_in_int = atoi((char *)ora_plecare + 3);
                          }
                          else
                          {
                            minute_plecare_in_int = atoi((char *)ora_plecare + 2);
                          }
                          minute_plecare_in_int += numar_minute_int;
                          while (minute_plecare_in_int >= 60)
                          {
                            minute_plecare_in_int -= 60;
                            ora_plecare_in_int++;
                          }
                          if (ora_plecare_in_int >= 24)
                          {
                            ora_plecare_in_int -= 24;
                          }
                          char noua_ora_plecare[8];
                          if (minute_plecare_in_int < 10)
                          {
                            sprintf(noua_ora_plecare, "%d:0%d", ora_plecare_in_int, minute_plecare_in_int);
                          }
                          else
                          {
                            sprintf(noua_ora_plecare, "%d:%d", ora_plecare_in_int, minute_plecare_in_int);
                          }
                          xmlSetProp(nodcurent_statie, (xmlChar *)"ora_plecare", (xmlChar *)noua_ora_plecare);
                        }
                        ora_sosire = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_sosire");
                        if (ora_sosire != NULL)
                        {
                          int ora_sosire_in_int = atoi((char *)ora_sosire);
                          int minute_sosire_in_int;
                          if (ora_sosire_in_int >= 10)
                          {
                            minute_sosire_in_int = atoi((char *)ora_sosire + 3);
                          }
                          else
                          {
                            minute_sosire_in_int = atoi((char *)ora_sosire + 2);
                          }
                          minute_sosire_in_int += numar_minute_int;
                          while (minute_sosire_in_int >= 60)
                          {
                            minute_sosire_in_int -= 60;
                            ora_sosire_in_int++;
                          }
                          if (ora_sosire_in_int >= 24)
                          {
                            ora_sosire_in_int -= 24;
                          }
                          char noua_ora_sosire[8];
                          if (minute_sosire_in_int < 10)
                          {
                            sprintf(noua_ora_sosire, "%d:0%d", ora_sosire_in_int, minute_sosire_in_int);
                          }
                          else
                          {
                            sprintf(noua_ora_sosire, "%d:%d", ora_sosire_in_int, minute_sosire_in_int);
                          }
                          xmlSetProp(nodcurent_statie, (xmlChar *)"ora_sosire", (xmlChar *)noua_ora_sosire);
                        }
                      }
                      if (strcmp((char *)nume_statie, statie) == 0)
                      {
                        am_trecut = 1;
                        intarziere = xmlGetProp(nodcurent_statie, (xmlChar *)"intarziere");
                        intarziere_actuala = atoi((char *)intarziere);
                        printf("%d\n", intarziere_actuala);
                        int intarziere_totala = numar_minute_int + intarziere_actuala;
                        sprintf(numar_minute, "%dmin", intarziere_totala);
                        xmlSetProp(nodcurent_statie, (xmlChar *)"intarziere", (xmlChar *)numar_minute);
                        ora_plecare = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_plecare");
                        if (ora_plecare != NULL)
                        {
                          int ora_plecare_in_int = atoi((char *)ora_plecare);
                          int minute_plecare_in_int;
                          if (ora_plecare_in_int >= 10)
                          {
                            minute_plecare_in_int = atoi((char *)ora_plecare + 3);
                          }
                          else
                          {
                            minute_plecare_in_int = atoi((char *)ora_plecare + 2);
                          }
                          minute_plecare_in_int += numar_minute_int;
                          while (minute_plecare_in_int >= 60)
                          {
                            minute_plecare_in_int -= 60;
                            ora_plecare_in_int++;
                          }
                          if (ora_plecare_in_int >= 24)
                          {
                            ora_plecare_in_int -= 24;
                          }
                          char noua_ora_plecare[8];
                          if (minute_plecare_in_int < 10)
                          {
                            sprintf(noua_ora_plecare, "%d:0%d", ora_plecare_in_int, minute_plecare_in_int);
                          }
                          else
                          {
                            sprintf(noua_ora_plecare, "%d:%d", ora_plecare_in_int, minute_plecare_in_int);
                          }
                          xmlSetProp(nodcurent_statie, (xmlChar *)"ora_plecare", (xmlChar *)noua_ora_plecare);
                        }
                        ora_sosire = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_sosire");
                        if (ora_sosire != NULL)
                        {
                          int ora_sosire_in_int = atoi((char *)ora_sosire);
                          int minute_sosire_in_int;
                          if (ora_sosire_in_int >= 10)
                          {
                            minute_sosire_in_int = atoi((char *)ora_sosire + 3);
                          }
                          else
                          {
                            minute_sosire_in_int = atoi((char *)ora_sosire + 2);
                          }
                          minute_sosire_in_int += numar_minute_int;
                          while (minute_sosire_in_int >= 60)
                          {
                            minute_sosire_in_int -= 60;
                            ora_sosire_in_int++;
                          }
                          if (ora_sosire_in_int >= 24)
                          {
                            ora_sosire_in_int -= 24;
                          }
                          char noua_ora_sosire[8];
                          if (minute_sosire_in_int < 10)
                          {
                            sprintf(noua_ora_sosire, "%d:0%d", ora_sosire_in_int, minute_sosire_in_int);
                          }
                          else
                          {
                            sprintf(noua_ora_sosire, "%d:%d", ora_sosire_in_int, minute_sosire_in_int);
                          }
                          xmlSetProp(nodcurent_statie, (xmlChar *)"ora_sosire", (xmlChar *)noua_ora_sosire);
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
        xmlSaveFormatFile("trenuri.xml", descriptor_fisier_xml, 1);
        if (am_trecut == 0)
        {
          strcpy(informatii_trenuri, "Indicativ tren inexistent/ Statie inexistenta!");
        }
        else
        {
          int numar_id_modificare_primit = atoi(id_modificare_primit);
          strcpy(coada_de_intarzieri[intarziere_vazuta].mesaj, comanda_primita);
          coada_de_intarzieri[intarziere_vazuta].indicativ_tren = numar_id_modificare_primit;
          strcpy(informatii_trenuri, "Actualizare efectuata cu succes!");
        }
        informatii_trenuri_dimensiune = strlen(informatii_trenuri);
        write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
        write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
        printf("%s\n", informatii_trenuri);
        printf("[Thread %d]Mesajul a fost transmis cu succes! \n", tdL.idThread);
        xmlFree(id_tren);
        xmlFree(ora_plecare);
        xmlFree(ora_sosire);
        xmlFree(nume_statie);
        xmlFree(intarziere);
        xmlFree(descriptor_fisier_xml);
        pthread_mutex_unlock(&mlock);
      }
      else
      {
        {
          strcpy(informatii_trenuri, "Niciun utilizator conectat!");
          informatii_trenuri_dimensiune = strlen(informatii_trenuri);
          printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, informatii_trenuri);
          write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
          write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
          printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
          printf("\n");
        }
      }
    }

    if (sscanf(comanda_primita, "mai devreme tren IR-%s %s min %s", id_modificare_primit, numar_minute, statie) == 3)
    {
      printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
      char mesaj[100] = {0};
      int mesaj_dimensiune;
      char informatii_trenuri[100] = {0};
      int informatii_trenuri_dimensiune;
      int am_trecut = 0;
      int numar_minute_int = atoi(numar_minute);
      int intarziere_actuala = 0;
      strcat(numar_minute, "min");
      strcpy(mesaj, "Am primit comanda - mai devreme tren IR-id_tren x min statie -");
      mesaj_dimensiune = strlen(mesaj);
      write(tdL.cl, &mesaj_dimensiune, sizeof(int));
      write(tdL.cl, mesaj, mesaj_dimensiune);
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
      // printf("%s\n", id_modificare_primit);
      // printf("%s\n", numar_minute);
      // printf("%s\n", statie);
      // printf("%d\n", numar_minute_int);
      if (logat != 0)
      {
        xmlNodePtr trenuri, nodcurent_tren, nodcurent_statie;
        xmlChar *id_tren, *nume_statie, *ora_plecare, *ora_sosire, *intarziere, *mai_devreme;
        pthread_mutex_lock(&mlock);
        descriptor_fisier_xml = xmlReadFile("trenuri.xml", NULL, 0);
        trenuri = xmlDocGetRootElement(descriptor_fisier_xml);
        for (nodcurent_tren = trenuri->children; nodcurent_tren; nodcurent_tren = nodcurent_tren->next)
        {
          if (nodcurent_tren->type == XML_ELEMENT_NODE)
          {
            if (xmlStrcmp(nodcurent_tren->name, (xmlChar *)"tren") == 0)
            {
              id_tren = xmlGetProp(nodcurent_tren, (xmlChar *)"id");
              if (xmlStrcmp(id_tren, (xmlChar *)id_modificare_primit) == 0)
              {
                for (nodcurent_statie = nodcurent_tren->children; nodcurent_statie; nodcurent_statie = nodcurent_statie->next)
                {
                  if (nodcurent_statie->type == XML_ELEMENT_NODE)
                  {
                    if (xmlStrcmp(nodcurent_statie->name, (xmlChar *)"statie") == 0)
                    {
                      nume_statie = xmlGetProp(nodcurent_statie, (xmlChar *)"nume");
                      if (am_trecut == 1)
                      {
                        intarziere = xmlGetProp(nodcurent_statie, (xmlChar *)"intarziere");
                        intarziere_actuala = atoi((char *)intarziere);
                        int intarziere_totala = intarziere_actuala - numar_minute_int;
                        sprintf(numar_minute, "%dmin", intarziere_totala);
                        xmlSetProp(nodcurent_statie, (xmlChar *)"intarziere", (xmlChar *)numar_minute);
                        ora_plecare = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_plecare");
                        if (ora_plecare != NULL)
                        {
                          int ora_plecare_in_int = atoi((char *)ora_plecare);
                          int minute_plecare_in_int;
                          if (ora_plecare_in_int >= 10)
                          {
                            minute_plecare_in_int = atoi((char *)ora_plecare + 3);
                          }
                          else
                          {
                            minute_plecare_in_int = atoi((char *)ora_plecare + 2);
                          }
                          minute_plecare_in_int -= numar_minute_int;
                          while (minute_plecare_in_int < 0)
                          {
                            minute_plecare_in_int += 60;
                            ora_plecare_in_int--;
                          }
                          if (ora_plecare_in_int < 0)
                          {
                            ora_plecare_in_int += 24;
                          }
                          char noua_ora_plecare[8];
                          if (minute_plecare_in_int < 10)
                          {
                            sprintf(noua_ora_plecare, "%d:0%d", ora_plecare_in_int, minute_plecare_in_int);
                          }
                          else
                          {
                            sprintf(noua_ora_plecare, "%d:%d", ora_plecare_in_int, minute_plecare_in_int);
                          }
                          xmlSetProp(nodcurent_statie, (xmlChar *)"ora_plecare", (xmlChar *)noua_ora_plecare);
                        }
                        ora_sosire = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_sosire");
                        if (ora_sosire != NULL)
                        {
                          int ora_sosire_in_int = atoi((char *)ora_sosire);
                          int minute_sosire_in_int;
                          if (ora_sosire_in_int >= 10)
                          {
                            minute_sosire_in_int = atoi((char *)ora_sosire + 3);
                          }
                          else
                          {
                            minute_sosire_in_int = atoi((char *)ora_sosire + 2);
                          }
                          minute_sosire_in_int -= numar_minute_int;
                          while (minute_sosire_in_int < 0)
                          {
                            minute_sosire_in_int += 60;
                            ora_sosire_in_int--;
                          }
                          if (ora_sosire_in_int < 0)
                          {
                            ora_sosire_in_int += 24;
                          }
                          char noua_ora_sosire[8];
                          if (minute_sosire_in_int < 10)
                          {
                            sprintf(noua_ora_sosire, "%d:0%d", ora_sosire_in_int, minute_sosire_in_int);
                          }
                          else
                          {
                            sprintf(noua_ora_sosire, "%d:%d", ora_sosire_in_int, minute_sosire_in_int);
                          }
                          xmlSetProp(nodcurent_statie, (xmlChar *)"ora_sosire", (xmlChar *)noua_ora_sosire);
                        }
                      }
                      if (strcmp((char *)nume_statie, statie) == 0)
                      {
                        am_trecut = 1;
                        intarziere = xmlGetProp(nodcurent_statie, (xmlChar *)"intarziere");
                        intarziere_actuala = atoi((char *)intarziere);
                        // printf("%d\n", intarziere_actuala);
                        int intarziere_totala = intarziere_actuala - numar_minute_int;
                        sprintf(numar_minute, "%dmin", intarziere_totala);
                        xmlSetProp(nodcurent_statie, (xmlChar *)"intarziere", (xmlChar *)numar_minute);
                        ora_plecare = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_plecare");

                        if (ora_plecare != NULL)
                        {
                          int ora_plecare_in_int = atoi((char *)ora_plecare);
                          int minute_plecare_in_int;
                          if (ora_plecare_in_int >= 10)
                          {
                            minute_plecare_in_int = atoi((char *)ora_plecare + 3);
                          }
                          else
                          {
                            minute_plecare_in_int = atoi((char *)ora_plecare + 2);
                          }
                          minute_plecare_in_int -= numar_minute_int;
                          while (minute_plecare_in_int < 0)
                          {
                            minute_plecare_in_int += 60;
                            ora_plecare_in_int--;
                          }
                          if (ora_plecare_in_int < 0)
                          {
                            ora_plecare_in_int += 24;
                          }
                          char noua_ora_plecare[8];
                          if (minute_plecare_in_int < 10)
                          {
                            sprintf(noua_ora_plecare, "%d:0%d", ora_plecare_in_int, minute_plecare_in_int);
                          }
                          else
                          {
                            sprintf(noua_ora_plecare, "%d:%d", ora_plecare_in_int, minute_plecare_in_int);
                          }
                          xmlSetProp(nodcurent_statie, (xmlChar *)"ora_plecare", (xmlChar *)noua_ora_plecare);
                        }
                        ora_sosire = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_sosire");
                        if (ora_sosire != NULL)
                        {
                          int ora_sosire_in_int = atoi((char *)ora_sosire);
                          int minute_sosire_in_int;
                          if (ora_sosire_in_int >= 10)
                          {
                            minute_sosire_in_int = atoi((char *)ora_sosire + 3);
                          }
                          else
                          {
                            minute_sosire_in_int = atoi((char *)ora_sosire + 2);
                          }
                          minute_sosire_in_int -= numar_minute_int;
                          while (minute_sosire_in_int < 0)
                          {
                            minute_sosire_in_int += 60;
                            ora_sosire_in_int--;
                          }
                          if (ora_sosire_in_int < 0)
                          {
                            ora_sosire_in_int += 24;
                          }
                          char noua_ora_sosire[8];
                          if (minute_sosire_in_int < 10)
                          {
                            sprintf(noua_ora_sosire, "%d:0%d", ora_sosire_in_int, minute_sosire_in_int);
                          }
                          else
                          {
                            sprintf(noua_ora_sosire, "%d:%d", ora_sosire_in_int, minute_sosire_in_int);
                          }
                          xmlSetProp(nodcurent_statie, (xmlChar *)"ora_sosire", (xmlChar *)noua_ora_sosire);
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
        xmlSaveFormatFile("trenuri.xml", descriptor_fisier_xml, 1);
        if (am_trecut == 0)
        {
          strcpy(informatii_trenuri, "Indicativ tren inexistent/ Statie inexistenta!");
        }
        else
        {
          int numar_id_modificare_primit = atoi(id_modificare_primit);
          strcpy(coada_de_intarzieri[intarziere_vazuta].mesaj, comanda_primita);
          coada_de_intarzieri[intarziere_vazuta].indicativ_tren = numar_id_modificare_primit;
          strcpy(informatii_trenuri, "Actualizare efectuata cu succes!");
        }
        informatii_trenuri_dimensiune = strlen(informatii_trenuri);
        write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
        write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
        printf("%s\n", informatii_trenuri);
        printf("[Thread %d]Mesajul a fost transmis cu succes! \n", tdL.idThread);
        xmlFree(id_tren);
        xmlFree(ora_plecare);
        xmlFree(ora_sosire);
        xmlFree(nume_statie);
        xmlFree(intarziere);
        xmlFree(mai_devreme);
        xmlFree(descriptor_fisier_xml);
        pthread_mutex_unlock(&mlock);
      }
      else
      {
        {
          strcpy(informatii_trenuri, "Niciun utilizator conectat!");
          informatii_trenuri_dimensiune = strlen(informatii_trenuri);
          printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, informatii_trenuri);
          write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
          write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
          printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
          printf("\n");
        }
      }
    }

    if (sscanf(comanda_primita, "trenuri %s - %s", localitate1, localitate2) == 2)
    {
      printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
      char mesaj[100] = {0};
      int mesaj_dimensiune;
      char informatii_trenuri[1200] = {0};
      int informatii_trenuri_dimensiune;
      int prima_localitate_marcat = 0;
      strcpy(mesaj, "Am primit comanda - trenuri localitate_1 - localitate_2 -");
      mesaj_dimensiune = strlen(mesaj);
      write(tdL.cl, &mesaj_dimensiune, sizeof(int));
      write(tdL.cl, mesaj, mesaj_dimensiune);
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
      if (logat != 0)
      {
        xmlNodePtr trenuri, nodcurent_tren, nodcurent_statie;
        xmlChar *id_tren, *nume_statie, *ora_plecare, *ora_sosire, *intarziere, *nume_statie_temp;
        pthread_mutex_lock(&mlock);
        descriptor_fisier_xml = xmlReadFile("trenuri.xml", NULL, 0);
        trenuri = xmlDocGetRootElement(descriptor_fisier_xml);
        for (nodcurent_tren = trenuri->children; nodcurent_tren; nodcurent_tren = nodcurent_tren->next)
        {
          if (nodcurent_tren->type == XML_ELEMENT_NODE)
          {
            if (xmlStrcmp(nodcurent_tren->name, (xmlChar *)"tren") == 0)
            {
              id_tren = xmlGetProp(nodcurent_tren, (xmlChar *)"id");
              for (nodcurent_statie = nodcurent_tren->children; nodcurent_statie; nodcurent_statie = nodcurent_statie->next)
              {
                if (nodcurent_statie->type == XML_ELEMENT_NODE)
                {
                  nume_statie = xmlGetProp(nodcurent_statie, (xmlChar *)"nume");
                  if (xmlStrcmp(nodcurent_statie->name, (xmlChar *)"statie") == 0)
                  {
                    if (strcmp(localitate1, (char *)nume_statie) == 0)
                    {
                      ora_plecare = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_plecare");
                      printf("%s\n", id_tren);
                      nume_statie_temp = xmlGetProp(nodcurent_statie, (xmlChar *)"nume");
                      prima_localitate_marcat = 1;
                    }
                    if (strcmp(localitate2, (char *)nume_statie) == 0 && prima_localitate_marcat == 1)
                    {
                      ora_sosire = xmlGetProp(nodcurent_statie, (xmlChar *)"ora_sosire");
                      prima_localitate_marcat = 0;
                      sprintf(informatii_trenuri + strlen(informatii_trenuri), "\ntren IR-%s\n", (char *)id_tren);
                      sprintf(informatii_trenuri + strlen(informatii_trenuri), "Pleaca din statia - %s\n", (char *)nume_statie_temp);
                      sprintf(informatii_trenuri + strlen(informatii_trenuri), "La ora: %s\n", (char *)ora_plecare);
                      sprintf(informatii_trenuri + strlen(informatii_trenuri), "Soseste in statia - %s\n", (char *)nume_statie);
                      sprintf(informatii_trenuri + strlen(informatii_trenuri), "La ora: %s\n", (char *)ora_sosire);
                    }
                  }
                }
              }
              prima_localitate_marcat = 0;
            }
          }
        }
        informatii_trenuri_dimensiune = strlen(informatii_trenuri);
        if (informatii_trenuri_dimensiune == 0)
        {
          strcpy(informatii_trenuri, "Nu am gasit rute disponibile!");
          informatii_trenuri_dimensiune = strlen(informatii_trenuri);
        }
        write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
        write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
        printf("%s\n", informatii_trenuri);
        printf("[Thread %d]Mesajul a fost transmis cu succes! \n", tdL.idThread);
        xmlFree(id_tren);
        xmlFree(ora_plecare);
        xmlFree(ora_sosire);
        xmlFree(nume_statie);
        xmlFree(intarziere);
        xmlFree(descriptor_fisier_xml);
        pthread_mutex_unlock(&mlock);
      }
      else
      {
        {
          strcpy(informatii_trenuri, "Niciun utilizator conectat!");
          informatii_trenuri_dimensiune = strlen(informatii_trenuri);
          printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, informatii_trenuri);
          write(tdL.cl, &informatii_trenuri_dimensiune, sizeof(int));
          write(tdL.cl, informatii_trenuri, informatii_trenuri_dimensiune);
          printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
          printf("\n");
        }
      }
    }

    if ((sscanf(comanda_primita, "trenuri %s - %s", localitate1, localitate2) != 2) && (sscanf(comanda_primita, "intarziere tren IR-%s %s min %s", id_modificare_primit, numar_minute, statie) != 3) && (sscanf(comanda_primita, "mai devreme tren IR-%s %s min %s", id_modificare_primit, numar_minute, statie) != 3) && (strcmp(comanda_primita, "sosiri trenuri urmatoarea ora") != 0) && (sscanf(comanda_primita, "notificari tren IR-%s", id_preferat_primit) != 1) && (strcmp(comanda_primita, "plecari trenuri urmatoarea ora") != 0) && (strncmp(comanda_primita, "tren IR-", 8) != 0) && (strcmp(comanda_primita, "quit") != 0) && (strncmp(comanda_primita, "sign up : ", 10) != 0) && strcmp(comanda_primita, "lista trenuri astazi") && (strncmp(comanda_primita, "login : ", 8) != 0) && (strcmp(comanda_primita, "logout") != 0) && (strlen(comanda_primita) > 0))
    {
      printf("[Thread %d]Mesajul a fost receptionat... %s\n", tdL.idThread, comanda_primita);
      char mesaj[100] = {0};
      int mesaj_dimensiune;
      strcpy(mesaj, "Comanda invalida!");
      mesaj_dimensiune = strlen(mesaj);
      write(tdL.cl, &mesaj_dimensiune, sizeof(int));
      write(tdL.cl, mesaj, mesaj_dimensiune);

      char mesaj2[200] = {0};
      int mesaj_dimensiune2;
      strcpy(mesaj2, "Consultati manualul de comenzi!");
      mesaj_dimensiune2 = strlen(mesaj2);
      printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, mesaj2);
      write(tdL.cl, &mesaj_dimensiune2, sizeof(int));
      write(tdL.cl, mesaj2, mesaj_dimensiune2);
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n", tdL.idThread);
      printf("\n");
    }
  }
}