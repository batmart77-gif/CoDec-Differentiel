# Codec Différentiel – Projet Compression d’Images

## Participants
- **Yani Tighilet**
- **Baptiste Martinez**

--- 

## Description du projet

Ce projet consiste à implémenter un **codec de compression d’images** basé sur un **codage différentiel** suivi d’une **normalisation** et d’un **codage VLC (Variable Length Coding)**, conformément au sujet fourni.

Le codec permet :
- d’encoder une image d'un format quelconque vers un format binaire **`.dif`**
- de décoder un fichier **`.dif`** pour reconstruire une image **PNM**

---

## Compilation

Compilation avec `make` :

```bash
make
```

Nettoyage avec `make clean` :
```bash
make clean
```

---

## Exemples d'exécution 

Encodage (PNM → DIF)
```bash
./codec -i boat.pgm -o boat.dif
``` 

Décodage (DIF -> PNM)
```bash
./codec -d -i boat.dif -o reconstructed.pnm
```

---

## Options de la ligne de commande

- "-i fichier" : fichier d'entrée
- "-o fichier" : fichier de sortie
- "-d" : active le mode décodage (DIF → PNM)
- "-t" : temps d'exécution des fonctions principal pnmtodif et diftopnm
- "-V"  : pour ouvrir l'image reconstruite avec votre visualiseur préféré, au moment de décoder
- "-h" : affiche l'aide
- "-v" : mode verbeux

---

## Répartition du travail 

Baptiste Martinez
- Lecture et écriture des images PNM (image.c)
- Transformation différentielles (image_to_dif)
- Normalisation / dénormalisation
- fonctions pushbits et pullbits
- fonction pnmtodif

Yani Tighilet 
- Implémentation du la VLC (encode / decode)
- Reconstruction de l'image (diff_to_image)
- ajout des différentes options 
- fonction diftopnm

---

## Ce qui marche dans notre projet : 
- Lecture de nombreux formats d'image via conversion automatique
- Codage et décodage VLC
- Calcul du taux de compression et du gain
- Transformation différentiel de l'image et reconstruction parfaite (image reconstruite quasi identique à l'original)
- Reconstruction parfaite des images directement au format DIF
- Format DIF conforme au sujets

## Ce qui ne marche pas dans notre projet : 
- Nous n'avons malheureusement pas réussi à garder les signatures imposées des fonctions pushbits et pullbits dans le sujet de TP. Nous avons tout testé, et les unsigned char provoquaient un changement de contraste sur les images, et parfois des traits horizontaux. 

--- 

## Difficultés rencontrées : 
- Difficultés au niveau des fonctions pushbits et pullbits. Comme avec un unsigned char on ne pouvait écrire ou lire que 8 bits, on avait une compression incorrect et plusieurs bugs puisque dans notre code on extrait des groupes de bits de taille variable qui peuvent dépasser 8 bits. 

- Dans le sujet il est écrit : "le premier pixel : valeur brute du premier pixel de l’image : 1 ou 3 octets."
c'est-à-dire la vraie valeur du pixel sans division par deux ou autre opération. Or dans notre fonction image_to_diff on fait ceci :
```c
int16_t val = (int16_t)(img->pixmap[c] / 2);
diff->data[c] = val;
```
Puis dans pnmtodif on écrit les premiers pixels avec :
```c
for (size_t c = 0; c < diff->layer; ++c) {
    unsigned char first = (unsigned char)(diff->data[c]);
    fwrite(&first, 1, 1, out);
}
```
donc diff->data[c] n'est pas la valeur brute du pixel puisqu'on divise par deux. 




