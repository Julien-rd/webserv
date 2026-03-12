#include <stdio.h>
#include <unistd.h>     // Für read(), close() und STDIN_FILENO (0)
#include <sys/epoll.h>  // Der Star der Show

#define MAX_EVENTS 5    // Wie viele Events wir maximal auf einmal verarbeiten wollen

int main() {
    int epoll_fd;
    struct epoll_event event;             // Konfiguration für EINEN File Descriptor
    struct epoll_event events[MAX_EVENTS]; // Array für die Ergebnisse von epoll_wait

    // 1. EPOLL ERSTELLEN (Den Pager kaufen)
    // epoll_create1(0) ist die moderne Version von epoll_create()
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Fehler bei epoll_create1");
        return 1;
    }

    // 2. DEN FILE DESCRIPTOR REGISTRIEREN (Tisch dem Pager hinzufügen)
    // Wir wollen die Standardeingabe (Tastatur, FD 0) überwachen
    event.events = EPOLLIN;          // EPOLLIN bedeutet: "Sag mir Bescheid, wenn Daten zum LESEN da sind"
    event.data.fd = STDIN_FILENO;    // STDIN_FILENO ist einfach 0

    // epoll_ctl fügt FD 0 zur epoll-Instanz hinzu (EPOLL_CTL_ADD)
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &event) == -1) {
        perror("Fehler bei epoll_ctl");
        return 1;
    }

    printf("epoll wartet jetzt auf deine Tastatureingabe...\n");

    // 3. DIE EVENT-SCHLEIFE (Der Kellner wartet auf ein Signal)
    while (1) {
        // epoll_wait blockiert das Programm, bis ein Event passiert.
        // -1 am Ende bedeutet: "Warte unendlich lange (kein Timeout)"
        int anzahl_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        if (anzahl_events == -1) {
            perror("Fehler bei epoll_wait");
            break;
        }

        // Gehe alle Events durch, die gerade passiert sind
        for (int i = 0; i < anzahl_events; i++) {
            
            // Wenn das Event von unserer Tastatur kommt (FD 0)
            if (events[i].data.fd == STDIN_FILENO) {
                char puffer[100];
                
                // Lese die Daten von der Tastatur
                ssize_t bytes_gelesen = read(STDIN_FILENO, puffer, sizeof(puffer) - 1);
                
                if (bytes_gelesen > 0) {
                    puffer[bytes_gelesen] = '\0'; // String sauber beenden
                    printf(">> epoll meldet: Du hast getippt: %s", puffer);
                }
            }
        }
    }

    // Aufräumen (wird in dieser Endlosschleife nie erreicht, aber guter Stil)
    close(epoll_fd);
    return 0;
}