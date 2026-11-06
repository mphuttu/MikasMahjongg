# MikasMahjongg

Windowsin MFC- ja Direct2D-mahjongg-pasianssi. Projekti tarvitsee Visual
Studio 2026:n C++- ja MFC-työkalut sekä Windows SDK:n. Peli on saanut
innoituksensa Ron Balewskin Mahjongg-pelistä.

**English:** MikasMahjongg is a Windows Mahjong solitaire game built with C++,
MFC, and Direct2D, inspired by Ron Balewski's Mahjongg. It features the classic
turtle layout, solvable deals, save and load, undo, hints, reshuffling, a Hall of
Fame, and Finnish and English interfaces and help. Build with Visual Studio
2026's C++ and MFC tools and the Windows SDK; the x64 installer is built with
Inno Setup 6. The sections below contain detailed build and playing instructions.

## Kääntäminen

Avaa
`MikasMahjongg.slnx` ja käännä esimerkiksi `Debug | x64`. Projekti kopioi
PNG-laatat suoritettavan tiedoston viereen hakemistoon `res\tiles` sekä
suomen- ja englanninkieliset CHM-ohjeet. Peli avaa valitun kielen ohjeen
valinnalla **Ohje > Ohjeen sisältö** tai **Help > Help Contents**.

## Kielivalinta ja asennus

64-bittiselle Windows 10:lle tai uudemmalle voi rakentaa asennusohjelman
polkuun `dist\MikasMahjongg-Setup-x64.exe` alla olevilla ohjeilla.
Rakennustuotokset ja asennusohjelma eivät kuulu lähdekoodirepositorioon.
Asennusohjelman kielivalinta määrittää
myös pelin kielen. Asennus on käyttäjäkohtainen eikä vaadi järjestelmänlaajuista
VC++-asennusta: tarvittavat x64 MFC- ja VC++-ajonaikaiset DLL:t asennetaan
pelin omaan kansioon. Windowsin omien dialogien ja järjestelmäpainikkeiden
kieli voi riippua Windowsin kieliasetuksista. MFC:n työkalurivien
mukautusikkuna on suomenkielisessäkin versiossa englanniksi.

Asennus tallentaa kielen ohjelmakansion `language.ini`-tiedostoon:
`[Language]`-osiossa `Code=fi` tai `Code=en`. Ilman asetustiedostoa
kehitysversio käyttää englantia. Kielen vaihtaminen edellyttää ohjelman
uudelleenkäynnistystä. Valikkojen ja työkalurivien mukautukset tallennetaan
kielikohtaisesti; kunniataulu on yhteinen.

## Ohjeiden ja asennusohjelman rakentaminen

Muokkaa tarvittaessa `help\fi.html`- ja `help\en.html`-lähteitä ja käännä
ohjeet HTML Help Workshopilla ennen sovellusta:

```powershell
& 'C:\Program Files (x86)\HTML Help Workshop\hhc.exe' 'help\fi.hhp'
& 'C:\Program Files (x86)\HTML Help Workshop\hhc.exe' 'help\en.hhp'
```

HTML Help Compiler käyttää onnistumiselle poistumiskoodia 1. Tarkista myös,
että `Ohje-fi.chm` ja `Help-en.chm` syntyivät ilman kääntäjän virheitä.
Käännä sitten sovellus asetuksella `Release | x64` ja suorita:

```powershell
& 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe' 'MikasMahjongg.iss'
```

Asennusskripti käyttää oletuksena Visual Studio 2026:n mukana tulevia
version 14.51.36231 x64-ajonaikaisia tiedostoja. Jos käytössä on toinen
työkalusarjan versio, anna vastaavan ajonaikaisen version x64-hakemisto
Inno Setupin `/DRedistRoot=...`-parametrilla. Sen on vastattava sovelluksen
käännöksessä käytettyä työkalusarjaa.

## Pelaaminen

Valitse hiirellä kaksi vapaata, yhteensopivaa laattaa. Laatta on vapaa,
kun sen päällä ei ole toista laattaa ja sen vasen tai oikea sivu on vapaa.
Kukat sopivat keskenään, kuten myös vuodenajat; muiden laattojen tulee
olla samanlaisia.
Saman kerroksen laattapinnat ovat kiinni toisissaan; näkyvät sivut ja
ylempien kerrosten siirtymä erottavat kerrokset.

- **File > New Game** (Ctrl+N): uusi peli.
- **File > Load Game / Save Game / Save As**: lataa tai tallenna peli
  `.mahjongg`-tiedostona. Tallennus sisältää peliajan, alkuperäisen jaon ja
  kumoamishistorian.
- **File > Restart Game** tai työkalurivin **Restart**: aloita sama jako alusta.
- **Edit > Undo** (Ctrl+Z) tai **Backup > Backup**: kumoa viimeisin pari.
- **Help > Show Moves**: korosta yksi mahdollinen pari.
- **Help > Tiles Left**: näytä jäljellä olevien laattojen määrä.
- **Help > Resuffle**: järjestä jäljellä olevat laatat uudelleen. Onnistunut
  sekoitus on ratkaistavissa ja tyhjentää kumoamishistorian. Jos nykyiseen
  muotoon ei löydy ratkaisujärjestystä, peli pysyy ennallaan; kumoa siirtoja
  ja yritä uudelleen.
- **Special > Pause** (Alt+P): pysäytä aika ja näytä taukoanimaatio.
- **Special > View Hall of Fame**: näytä kymmenen nopeinta voittoa.
  **Clear Hall of Fame** poistaa koko tuloslistan vahvistuksen jälkeen.

Kaikki voitot kelpaavat tuloslistalle myös vihjeitä, kumoamista ja sekoitusta
käytettäessä. Tulokset tallennetaan nykyisen Windows-käyttäjän rekisteriin.
Keskeneräisen pelin sulkeminen tai vaihtaminen kysyy tallennetaanko se.

Uusi peli ja onnistunut sekoitus muodostetaan poistamalla vapaita paikkapareja
ja jakamalla yhteensopivat laatat tällä poistojärjestyksellä. Alkuasetelmalla
on siis vahvistettu ratkaisu, mutta kaikki pelaajan tekemien siirtojen jälkeiset
asemat eivät välttämättä ole ratkaistavissa. Vihje näyttää mahdollisen siirron,
ei välttämättä voittoon johtavaa siirtoa.

Äänet ja muut harmaat valikkotoiminnot eivät vielä kuulu tähän versioon.
Vanhan pelin suoritettavaa tiedostoa tai lähdekoodia ei toimiteta tämän
projektin mukana.

## Pelilogiikan testit

Esimerkiksi MinGW:n komentokehotteessa:

```text
g++ -std=c++14 -O2 MahjongGame.cpp tests\MahjongGameTests.cpp -o MahjongGameTests.exe
MahjongGameTests.exe
```

Testit tarkistavat tallennuksen, latauksen ja uudelleenaloituksen sekä
kilpikonna-asetelman, laattajakauman, vapaussäännöt ja
100 uuden pelin sekä uudelleensekoituksen poistoketjut. Ne tarkistavat myös
laattojen kerrossiirtymät ja sen, että asetelma mahtuu erikokoisiin näkymiin.

Hall of Famen testit voi kääntää Visual Studion x64 Native Tools
-komentokehotteessa:

```text
cl /nologo /utf-8 /EHsc /MDd /D_DEBUG /D_AFXDLL /D_UNICODE /DUNICODE /I. HallOfFame.cpp Localization.cpp tests\HallOfFameTests.cpp /FeHallOfFameTests.exe
HallOfFameTests.exe
```

Testi käyttää omaa väliaikaista rekisteriavaintaan eikä muuta pelin tuloslistaa.

## Tekijätiedot

Copyright (c) Mika Huttunen. Mahjong tileset designed by Code Inferno
([www.codeinferno.com](http://www.codeinferno.com)).
Lähde: [OpenGameArt.org](https://opengameart.org/content/mahjong-tileset).
Laattojen lisenssi on [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/).
Lisätiedot toimitetaan myös tiedostossa `CREDITS.txt`.
