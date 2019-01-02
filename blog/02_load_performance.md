# Ucitavanje slike i merenje performansi
[<< Nazad na pocetak](../README.md)\
Datum: 29.12.2018.
Tag: v0.2

U ovom koraku je modifikovan hardver sa sledecim izmenama:
- dodat je Performance counter

Unete izmene u softveru su:
- ukljucen hostfs

Softverski je testirano ucitavanje dimenzija ulazne slike i same slike. Isti test je sproveden na iznaznoj slici. Izmerene su perfomanse sistema radi demonstriranja rada IP bloka.

Problem koji se pojavio je restartovanje sistema pri ucitavanju slike. Uzrok problema je sto je u proslom koraku pri kreiranju aplikacije izabran Hello world small umesto Hello world. Tom izmenom je problem resen.

Rezultat ove faze je sistem koji na specificiran nacin ucitava i ispisuje sliku i pri tome meri performanse. U sledecem koraku ce se implementirati algoritam racunanja histograma i ekvalizacije histograma.
