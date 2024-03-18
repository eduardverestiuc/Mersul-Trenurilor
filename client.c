#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <poll.h>
#include <signal.h>
extern int errno;

int port;

int logat = 0;

void sigchld_handler(int signo)
{
  (void)signo;

  logat = 0;
}

int main(int argc, char *argv[])
{
  int sd;
  struct sockaddr_in server;
  char buf[200] = {0};

  if (argc != 3)
  {
    printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    return -1;
  }

  port = atoi(argv[2]);

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Eroare la socket().\n");
    return errno;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_port = htons(port);

  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[client]Eroare la connect().\n");
    return errno;
  }

  printf("[client]Conectare | Inregistrare | Manual comenzi: \n");
  while (1)
  {
    // printf("Formatul: login : username \n");
    memset(&buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';
    printf("[client]Am citit: %s\n", buf);
    if (strcmp(buf, "Manual comenzi") == 0)
    {
      printf("[client]Pana la conectare, lista comenzilor este: \nlogin : username (ulterior se va solicita parola) \nsign up : username (ulterior se va solicita noua parola) \nlogout \nquit (se va inchide thread-ul respectiv din server) \n");
      printf("\n");
      printf("[client]Dupa conectare, lista comenzilor devine: \ntren IR-id_tren(un numar de 3 cifre) \nplecari trenuri urmatoarea ora \nsosiri trenuri urmatoarea ora \nintarziere tren IR-id_tren x min statie \nmaidevreme tren IR-id_tren x min statie \ntrenuri localitate_1 - localitate_2 \nlista trenuri astazi \nnotificari tren IR-id_tren");
      printf("\n");
      printf("Introduceti comanda: \n");
    }
    else
    {
      int dimensiune_buffer = strlen(buf);
      write(sd, &dimensiune_buffer, sizeof(int));
      write(sd, buf, dimensiune_buffer);

      char confirmare_comanda[200] = {0};
      int dimensiune_confirmare_comanda;
      if (logat == 0)
      {
        memset(&confirmare_comanda, 0, sizeof(confirmare_comanda));
        read(sd, &dimensiune_confirmare_comanda, sizeof(int));
        read(sd, confirmare_comanda, dimensiune_confirmare_comanda);
      }

      if (strcmp(confirmare_comanda, "Am primit comanda - login -") == 0)
      {
        printf("[server]%s\n", confirmare_comanda);
        char raspuns_server[200] = {0};
        int dimensiune_raspuns_server;
        read(sd, &dimensiune_raspuns_server, sizeof(int));
        read(sd, raspuns_server, dimensiune_raspuns_server);
        printf("[server]Mesajul primit este: %s\n", raspuns_server);
        if (strcmp(raspuns_server, "Numele de utilizator este corect!") == 0)
        {
          printf("parola: \n");
          char parola[200] = {0};
          int dimensiune_parola;
          fgets(parola, sizeof(parola), stdin);
          parola[strlen(parola) - 1] = '\0';
          dimensiune_parola = strlen(parola);
          // printf("%i\n", dimensiune_parola);
          write(sd, &dimensiune_parola, sizeof(int));
          write(sd, parola, dimensiune_parola);
          char raspuns_parola[200] = {0};
          int dimensiune_raspuns_parola;
          dimensiune_raspuns_parola = strlen(raspuns_parola);
          read(sd, &dimensiune_raspuns_parola, sizeof(int));
          read(sd, raspuns_parola, dimensiune_raspuns_parola);
          printf("[server]Mesajul primit este: %s\n", raspuns_parola);
          printf("\nIntroduceti comanda:\n");
          if (strcmp(raspuns_parola, "Utilizator conectat cu succes!") == 0)
          {
            signal(SIGCHLD, sigchld_handler);
            logat = 1;
            pid_t pid = fork();
            if (pid == 0)
            {
              while (1)
              {
                memset(&confirmare_comanda, 0, sizeof(confirmare_comanda));
                read(sd, &dimensiune_confirmare_comanda, sizeof(int));
                read(sd, confirmare_comanda, dimensiune_confirmare_comanda);
                printf("[server]%s\n", confirmare_comanda);
                if ((strncmp(confirmare_comanda, "intarziere", 10)) && (strncmp(confirmare_comanda, "mai devreme", 11)))
                {
                  char raspuns_server[2200] = {0};
                  int dimensiune_raspuns_server = 0;
                  read(sd, &dimensiune_raspuns_server, sizeof(int));
                  read(sd, raspuns_server, dimensiune_raspuns_server);
                  printf("[server]Mesajul primit este: %s\n", raspuns_server);
                  printf("\nIntroduceti comanda:\n");
                }
                else
                {
                 printf("\n"); 
                }
                if (strcmp(confirmare_comanda, "Am primit comanda - logout -") == 0)
                   {
                    exit(0);
                   }
              }
            }
          }
        }
        else
        {
          printf("\nIntroduceti comanda:\n");
          // printf("[client]Mesajul primit este: %s\n", raspuns_server);
        }
      }

      if (strcmp(confirmare_comanda, "Am primit comanda - sign up : username -") == 0)
      {
        printf("[server]%s\n", confirmare_comanda);
        char raspuns_server[200] = {0};
        int dimensiune_raspuns_server;
        read(sd, &dimensiune_raspuns_server, sizeof(int));
        read(sd, raspuns_server, dimensiune_raspuns_server);
        printf("[server]Mesajul primit este: %s\n", raspuns_server);
        if (strcmp(raspuns_server, "Nume de utilizator inregistrat cu succes!") == 0)
        {
          printf("parola: \n");
          char parola[200] = {0};
          int dimensiune_parola;
          fgets(parola, sizeof(parola), stdin);
          parola[strlen(parola) - 1] = '\0';
          dimensiune_parola = strlen(parola);
          // printf("%i\n", dimensiune_parola);
          write(sd, &dimensiune_parola, sizeof(int));
          write(sd, parola, dimensiune_parola);
          char raspuns_parola[200] = {0};
          int dimensiune_raspuns_parola;
          dimensiune_raspuns_parola = strlen(raspuns_parola);
          read(sd, &dimensiune_raspuns_parola, sizeof(int));
          read(sd, raspuns_parola, dimensiune_raspuns_parola);
          printf("[server]Mesajul primit este: %s\n", raspuns_parola);
          printf("\nIntroduceti comanda:\n");
        }
        else
        {
          printf("\nIntroduceti comanda:\n");
        }
      }

      if (strcmp(confirmare_comanda, "Am primit comanda - logout -") == 0)
      {
        printf("[server]%s\n", confirmare_comanda);
        char raspuns_server[200] = {0};
        int dimensiune_raspuns_server = 0;
        // printf("%i\n",100);
        memset(&dimensiune_raspuns_server, 0, sizeof(dimensiune_raspuns_server));
        read(sd, &dimensiune_raspuns_server, sizeof(int));
        memset(&raspuns_server, 0, sizeof(raspuns_server));
        // printf("%i\n", dimensiune_raspuns_server);
        read(sd, raspuns_server, dimensiune_raspuns_server);
        // int f1 = strlen(raspuns_server);
        // printf("%d\n",f1);
        printf("[server]Mesajul primit este: %s\n", raspuns_server);
        printf("\nIntroduceti comanda:\n");
      }

      if (strcmp(confirmare_comanda, "Am primit comanda - quit -") == 0)
      {
        printf("[server]La revedere!\n");
        exit(0);
      }

      if (strcmp(confirmare_comanda, "Am primit comanda - lista trenuri astazi -") == 0)
      {
        printf("[server]%s\n", confirmare_comanda);
        char raspuns_server[2200] = {0};
        int dimensiune_raspuns_server = 0;
        memset(&dimensiune_raspuns_server, 0, sizeof(dimensiune_raspuns_server));
        read(sd, &dimensiune_raspuns_server, sizeof(int));
        memset(&raspuns_server, 0, sizeof(raspuns_server));
        read(sd, raspuns_server, dimensiune_raspuns_server);
        printf("[server]Mesajul primit este: %s\n", raspuns_server);
        printf("Introduceti comanda:\n");
      }

      if (strcmp(confirmare_comanda, "Am primit comanda - tren IR-id_tren -") == 0)
      {
        printf("[server]%s\n", confirmare_comanda);
        char raspuns_server[1200] = {0};
        int dimensiune_raspuns_server = 0;
        memset(&dimensiune_raspuns_server, 0, sizeof(dimensiune_raspuns_server));
        read(sd, &dimensiune_raspuns_server, sizeof(int));
        memset(&raspuns_server, 0, sizeof(raspuns_server));
        read(sd, raspuns_server, dimensiune_raspuns_server);
        printf("[server]Mesajul primit este: %s\n", raspuns_server);
        printf("\nIntroduceti comanda:\n");
      }

      if (strcmp(confirmare_comanda, "Am primit comanda - plecari trenuri urmatoarea ora -") == 0)
      {
        printf("[server]%s\n", confirmare_comanda);
        char raspuns_server[1200] = {0};
        int dimensiune_raspuns_server = 0;
        memset(&dimensiune_raspuns_server, 0, sizeof(dimensiune_raspuns_server));
        read(sd, &dimensiune_raspuns_server, sizeof(int));
        memset(&raspuns_server, 0, sizeof(raspuns_server));
        read(sd, raspuns_server, dimensiune_raspuns_server);
        printf("[server]Mesajul primit este: %s\n", raspuns_server);
        printf("\nIntroduceti comanda:\n");
      }

      if (strcmp(confirmare_comanda, "Am primit comanda - sosiri trenuri urmatoarea ora -") == 0)
      {
        printf("[server]%s\n", confirmare_comanda);
        char raspuns_server[1200] = {0};
        int dimensiune_raspuns_server = 0;
        memset(&dimensiune_raspuns_server, 0, sizeof(dimensiune_raspuns_server));
        read(sd, &dimensiune_raspuns_server, sizeof(int));
        memset(&raspuns_server, 0, sizeof(raspuns_server));
        read(sd, raspuns_server, dimensiune_raspuns_server);
        printf("[server]Mesajul primit este: %s\n", raspuns_server);
        printf("\nIntroduceti comanda:\n");
      }

      if (strcmp(confirmare_comanda, "Am primit comanda - intarziere tren IR-id_tren x min statie -") == 0)
      {
        printf("[server]%s\n", confirmare_comanda);
        char raspuns_server[100] = {0};
        int dimensiune_raspuns_server = 0;
        memset(&dimensiune_raspuns_server, 0, sizeof(dimensiune_raspuns_server));
        read(sd, &dimensiune_raspuns_server, sizeof(int));
        memset(&raspuns_server, 0, sizeof(raspuns_server));
        read(sd, raspuns_server, dimensiune_raspuns_server);
        printf("[server]Mesajul primit este: %s\n", raspuns_server);
        printf("\nIntroduceti comanda:\n");
      }

      if (strcmp(confirmare_comanda, "Am primit comanda - mai devreme tren IR-id_tren x min statie -") == 0)
      {
        printf("[server]%s\n", confirmare_comanda);
        char raspuns_server[100] = {0};
        int dimensiune_raspuns_server = 0;
        memset(&dimensiune_raspuns_server, 0, sizeof(dimensiune_raspuns_server));
        read(sd, &dimensiune_raspuns_server, sizeof(int));
        memset(&raspuns_server, 0, sizeof(raspuns_server));
        read(sd, raspuns_server, dimensiune_raspuns_server);
        printf("[server]Mesajul primit este: %s\n", raspuns_server);
        printf("\nIntroduceti comanda:\n");
      }

      if (strcmp(confirmare_comanda, "Am primit comanda - notificari tren IR-id_tren -") == 0)
      {
        printf("[server]%s\n", confirmare_comanda);
        char raspuns_server[100] = {0};
        int dimensiune_raspuns_server = 0;
        memset(&dimensiune_raspuns_server, 0, sizeof(dimensiune_raspuns_server));
        read(sd, &dimensiune_raspuns_server, sizeof(int));
        memset(&raspuns_server, 0, sizeof(raspuns_server));
        read(sd, raspuns_server, dimensiune_raspuns_server);
        printf("[server]Mesajul primit este: %s\n", raspuns_server);
        printf("\nIntroduceti comanda:\n");
      }

      if (strcmp(confirmare_comanda, "Am primit comanda - trenuri localitate_1 - localitate_2 -") == 0)
      {
        printf("[server]%s\n", confirmare_comanda);
        char raspuns_server[1200] = {0};
        int dimensiune_raspuns_server = 0;
        memset(&dimensiune_raspuns_server, 0, sizeof(dimensiune_raspuns_server));
        read(sd, &dimensiune_raspuns_server, sizeof(int));
        memset(&raspuns_server, 0, sizeof(raspuns_server));
        read(sd, raspuns_server, dimensiune_raspuns_server);
        printf("[server]Mesajul primit este: %s\n", raspuns_server);
        printf("\nIntroduceti comanda:\n");
      }

      if (strcmp(confirmare_comanda, "Comanda invalida!") == 0)
      {
        printf("[server]%s\n", confirmare_comanda);
        char raspuns_server[1200] = {0};
        int dimensiune_raspuns_server = 0;
        memset(&dimensiune_raspuns_server, 0, sizeof(dimensiune_raspuns_server));
        read(sd, &dimensiune_raspuns_server, sizeof(int));
        memset(&raspuns_server, 0, sizeof(raspuns_server));
        read(sd, raspuns_server, dimensiune_raspuns_server);
        printf("[server]Mesajul primit este: %s\n", raspuns_server);
        printf("\nIntroduceti comanda:\n");
      }
    }
  }
  close(sd);
}