#include "tlv.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

//read nie zawsze w tcp odsyła całosc bajtów na raz, wiec trzeba funkcje która wywoła read tyle razy, az otrzyma wszystkie potrzebne bajty
//bo robimy to binarnie wiec nie chcemy zeby nam ucielo
static ssize_t readtlv(int desc2, void *buf, size_t n){
    ssize_t amount=n; //ilosc bajtów jaka została do odczytania 
    ssize_t r;
    char *ptr=buf; //gdzie zapisywac bajty w buforze

    while(amount>0){
        r=read(desc2, ptr, amount);
        if(r<0){
            if (errno == EINTR)  //jesli read przerwane to wroc do petli
                {continue;}
            return -1; //jesli blad to wyrzuc -1
        }
        else if(r==0){
            return n-amount; //zwraca ile bajtow odczytalismy gdy skonczylo sie połaczenie
        }
        amount = amount -r; //ile jeszcze bajtow do odczytania
        ptr = ptr + r; //zmiena wskaznik o ilosc bitow
    }
    return n; //zwraca tyle ile chcielismy odczytac jesli przeszedl caly while
}

static ssize_t writetlv(int desc2, void *buf, size_t n){
    ssize_t amount=n; //ilosc bajtów jaka została do zapisania
    ssize_t w;
    char *ptr=buf; //analogicznie do readtlv tylko zapis

    while(amount>0){
        w=write(desc2, ptr, amount);
        if(w<=0){
            if (w < 0 && errno == EINTR) //jesli przerwano wroc do petli
                {continue;} 
            return -1; //blad, teraz tez bierzemy pod uwage jesli w==0
        }
        amount=amount-w;
        ptr=ptr+w; 
    }
    return n; //jesli cala petla przeszla to powinnimsy zwracac tyle bajtow ile chcielismy zapisac
}

//tutaj juz kompletny komunikat type, lenght, value, type to: login, itd, dane i liczba bajtow
int sendtlv(int desc2, uint16_t type, const void *data, uint16_t len){
    struct tlv_hdr hdr; //tworzymy strukture nagłówka z protocol.h

    hdr.type=htons(type); //porzadek sieciowy
    hdr.length=htons(len);

    //wysylanie nagłowka
    if(writetlv(desc2, &hdr, sizeof(hdr)) != sizeof(hdr)){
        return -1; //jesli nie odebralo calego naglowka to blad
    }
    //wysyłanie danych
    if(len>0&& writetlv(desc2, data, len)!= len){
        return -1;
    } //jesli sa dane i ich nie wysalno calych to blad, (join np nie ma danych wiec spoko)
    return 0;
}

//odbieranie komunikaty - CAŁEGO. bufsize to max liczba bajtow, ktore mozemy zapisac- ograniczenie
int recv_tlv(int desc2, uint16_t *type, void *buf, uint16_t bufsize){
    struct tlv_hdr hdr; //tworzymy strukture nagłówka z protocol.h
    ssize_t r;
    //czytanie nagłowka
    r=readtlv(desc2, &hdr, sizeof(hdr)); //chcemy wszystko naraz wiec korzystamy z readtlv
    if(r<=0){
        return r; //albo 0 albo bład
    }
    *type = ntohs(hdr.type); //odczyt typu
    uint16_t len = ntohs(hdr.length); //odczyt długodci danych
    
    if(len>bufsize){
        return -1; // jak za długie tpo blad
    }
    if (len > 0) { //czy sa dahne
        //czytanie danych
        r = readtlv(desc2, buf, len); //odczyt danych o długosci len i zapis do bufora
        if (r != len)
           {return -1; } //jesli nie odczytało całych danych to blad
    }
    return len;
}

