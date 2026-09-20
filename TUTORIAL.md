# Tutoriel : apprendre à utiliser NoteEditor

Ce didacticiel vous guide pas à pas, de la première ouverture jusqu'aux
fonctions avancées. Chaque partie contient des manipulations **« À essayer »** :
faites-les au fur et à mesure, c'est le moyen le plus rapide d'apprendre.

NoteEditor est un éditeur de texte à onglets dont l'idée centrale est simple :
**vous n'avez jamais à vous soucier de sauvegarder ou de perdre votre travail.**
Vous écrivez, vous fermez, et tout est retrouvé à la réouverture.

> Il existe deux versions de l'application, **Python** et **C++**. Elles se
> comportent exactement de la même façon et partagent les mêmes données : tout ce
> qui est décrit ici vaut pour les deux.

## Sommaire

1. [Lancer l'application](#1-lancer-lapplication)
2. [Découvrir la fenêtre](#2-découvrir-la-fenêtre)
3. [Écrire votre première note](#3-écrire-votre-première-note)
4. [La restauration de session](#4-la-restauration-de-session)
5. [Travailler avec plusieurs onglets](#5-travailler-avec-plusieurs-onglets)
6. [Enregistrer et ouvrir des fichiers](#6-enregistrer-et-ouvrir-des-fichiers)
7. [Le panneau Brouillons](#7-le-panneau-brouillons)
8. [Épingler une note](#8-épingler-une-note)
9. [La corbeille](#9-la-corbeille)
10. [L'historique des versions](#10-lhistorique-des-versions)
11. [Rechercher et remplacer](#11-rechercher-et-remplacer)
12. [Confort d'écriture](#12-confort-décriture)
13. [Fichier modifié par un autre programme](#13-fichier-modifié-par-un-autre-programme)
14. [Lancer l'application au démarrage de la session](#14-lancer-lapplication-au-démarrage-de-la-session)
15. [Où sont mes données ?](#15-où-sont-mes-données-)
16. [Aide-mémoire des raccourcis](#16-aide-mémoire-des-raccourcis)
17. [Questions fréquentes](#17-questions-fréquentes)

---

## 1. Lancer l'application

**Version Python** (nécessite Python 3 et PyQt6 : `pip install -r requirements.txt`) :

```bash
cd Python
python3 main.py
```

**Version C++** (nécessite `qt6-base-dev`, un compilateur C++17 et CMake) :

```bash
cd Cpp
cmake -B build
cmake --build build -j"$(nproc)"
./build/NoteEditor
```

Vous n'avez besoin que d'une des deux. Le détail de l'installation est dans le
`README.md`.

Au tout premier lancement, l'application s'ouvre sur **une note vide**, déjà
prête à recevoir du texte.

---

## 2. Découvrir la fenêtre

```
┌────────────────────────────────────────────────────────────────────────┐
│ Fichier  Édition  Rechercher  Aide                                 (1) │
├────────────────────────────────────────────────────────────────────────┤
│ [Nouveau] [Ouvrir] [Enregistrer] […]                               (2) │
├──────────────────────┬─────────────────────────────────────────────────┤
│ Brouillons  (3)      │ [ 260919_07352346 ✕ ][ 260919_0741… ✕ ][+] (4)  │
│ [Rechercher…][Date ▾]├─────────────────────────────────────────────────┤
│ 260919_0735… (ouvert)│  1 │ Le texte de la note…                       │
│ 260919_0741… (ouvert)│  2 │                                            │
│                      │    │                                        (5) │
│ [Corbeille...]       │    │                                            │
├──────────────────────┴─────────────────────────────────────────────────┤
│ Ligne 1, Colonne 1   0 mot, 0 caractère   UTF-8                    (6) │
└────────────────────────────────────────────────────────────────────────┘
```

1. **Les menus** : Fichier, Édition, Rechercher, Aide. Toutes les actions y sont,
   avec leur raccourci clavier.
2. **La barre d'outils** : les actions les plus courantes en un clic. De gauche à
   droite : **Nouveau**, **Ouvrir**, **Enregistrer**, **Enregistrer sous**,
   **Fermer l'onglet**, **Épingler**, **Détacher**, **Corbeille**, **Rechercher**,
   **Rechercher / Remplacer**, **Retour automatique à la ligne** et **Aperçu
   Markdown**. Laissez la
   souris un instant sur une icône : une infobulle donne son nom.
3. **Le panneau Brouillons** : la liste de toutes vos notes, ouvertes ou non
   (voir la [partie 7](#7-le-panneau-brouillons)).
4. **La barre d'onglets** : une note par onglet, avec le bouton **+** juste après
   le dernier.
5. **La zone d'édition** : avec les numéros de ligne à gauche, et la ligne où se
   trouve le curseur légèrement surlignée.
6. **La barre de statut** : la position du curseur, le nombre de mots et de
   caractères, et l'encodage.

Le séparateur entre le panneau et la zone d'édition se déplace à la souris. Sa
position, comme la taille et l'emplacement de la fenêtre, sera retenue.

---

## 3. Écrire votre première note

**À essayer**

1. Cliquez dans la zone d'édition et tapez quelques lignes.
2. Regardez la **barre de statut** : la ligne, la colonne, le nombre de mots et de
   caractères changent en direct.
3. Regardez le **nom de l'onglet** : il ressemble à `260919_07352346`. C'est la
   date et l'heure de création de la note, au format `aammjj_hhmmssmm` (année,
   mois, jour, heure, minute, seconde, centièmes). Chaque nouvelle note reçoit
   ainsi un nom unique, sans que vous ayez à en choisir un.
4. Utilisez les raccourcis habituels : `Ctrl+Z` annuler, `Ctrl+Maj+Z` rétablir,
   `Ctrl+X` / `Ctrl+C` / `Ctrl+V` couper, copier, coller, `Ctrl+A` tout
   sélectionner (ils sont aussi dans le menu **Édition**).

**Bon à savoir :** votre texte est **archivé automatiquement** environ une
seconde et demie après votre dernière frappe. Vous n'avez rien à faire pour ne
pas le perdre.

**Renommer une note** : clic droit sur son onglet → **Renommer...**, puis tapez
un nom. (Cela ne concerne que les notes qui ne sont pas encore liées à un vrai
fichier, voir la [partie 6](#6-enregistrer-et-ouvrir-des-fichiers).)

---

## 4. La restauration de session

C'est la fonction principale de NoteEditor. À la fermeture, l'application retient
**tout** ; à la réouverture, vous retrouvez votre travail comme vous l'aviez
laissé.

**À essayer**

1. Créez trois notes avec `Ctrl+N` (ou le bouton **+**) et écrivez un mot
   différent dans chacune.
2. Placez-vous sur la deuxième note.
3. **Fermez complètement l'application** (croix de la fenêtre, `Ctrl+Q`, ou menu
   Fichier → Quitter). Aucune question ne vous est posée.
4. Relancez l'application.

Vous retrouvez :

- **les trois onglets**, dans le même ordre ;
- **leur texte**, même s'il n'a jamais été enregistré dans un fichier ;
- **l'onglet actif** : vous êtes de nouveau sur la deuxième note ;
- **la taille et la position de la fenêtre**, et la position du séparateur du
  panneau Brouillons ;
- l'**état « épinglé »** de vos notes (voir la [partie 8](#8-épingler-une-note)).

Si une note a des modifications non enregistrées dans son fichier, son nom
commence par une étoile `*` (par exemple `*rapport.txt`). Cette étoile est elle
aussi conservée.

Ce qui n'est pas retenu : la position du curseur dans chaque note, l'historique
annuler/rétablir, et l'option de retour à la ligne (activée à chaque lancement).

**Et si l'ordinateur plante ?** Le texte est archivé pendant que vous tapez : au
pire, vous perdez les toutes dernières secondes. Après un arrêt brutal, la liste
des onglets rouverts est celle de la dernière fermeture normale, mais **le texte
le plus récent de chacune de vos notes est conservé** : retrouvez-la dans le
panneau Brouillons (partie 7).

**Une garantie importante :** cette sauvegarde automatique **n'écrase jamais vos
vrais fichiers**. Elle écrit uniquement dans le dossier de l'application. Seule
une action volontaire de votre part (« Enregistrer ») modifie un fichier.

---

## 5. Travailler avec plusieurs onglets

### Créer, changer, réorganiser

- **Nouvelle note** : `Ctrl+N`, l'icône **Nouveau**, ou le bouton **+** à droite du
  dernier onglet.
- **Changer d'onglet** : cliquez sur son nom.
- **Réorganiser** : faites glisser un onglet à gauche ou à droite des autres.
- Quand il y a trop d'onglets pour la largeur de la fenêtre, des petites flèches
  de défilement apparaissent, et le bouton **+** se place à côté d'elles.

### Fermer un onglet

- La **croix** de l'onglet, `Ctrl+W`, ou l'icône **Fermer l'onglet**.
- **Il n'y a jamais de confirmation.** Fermer un onglet ne détruit pas la note :
  elle reste dans le panneau Brouillons et vous pouvez la rouvrir quand vous
  voulez.
- Vous pouvez fermer le dernier onglet : l'application reste ouverte, sans note.

### Le menu contextuel des onglets

Faites un **clic droit sur un onglet** :

| Entrée | Effet |
|---|---|
| **Fermer** | Ferme cet onglet |
| **Fermer les autres** | Ferme tous les onglets sauf celui-ci |
| **Fermer à droite** | Ferme les onglets situés à droite de celui-ci |
| **Fermer tout** | Ferme tous les onglets |
| **Épingler** / **Détacher** | Bloque ou débloque la fermeture (partie 8) |
| **Dupliquer** | Crée une nouvelle note avec le même texte |
| **Renommer...** | Change le nom (notes sans fichier associé) |
| **Historique des versions...** | Affiche les anciennes versions (partie 10) |
| **Mettre à la corbeille** | Met la note à la corbeille (partie 9) |

**À essayer :** créez quatre onglets, faites un clic droit sur le deuxième, puis
choisissez **Fermer à droite**. Il reste deux onglets.

---

## 6. Enregistrer et ouvrir des fichiers

Une note qui vient d'être créée n'est **pas liée à un fichier** : elle vit
uniquement dans l'application (et est archivée automatiquement). Vous avez deux
façons d'en faire un vrai fichier.

### Enregistrer (`Ctrl+S`)

- Sur une **note sans fichier**, `Ctrl+S` l'enregistre **directement**, sans
  aucune boîte de dialogue, dans le dossier `~/.noteeditor/docs/`, sous son nom
  par défaut avec l'extension `.txt` (par exemple `260919_07352346.txt`). Le nom
  de l'onglet devient alors celui du fichier.
- Sur une note **déjà liée à un fichier**, `Ctrl+S` réécrit ce fichier.

### Enregistrer sous (`Ctrl+Maj+S`)

Ouvre la fenêtre habituelle pour **choisir l'emplacement et le nom** du fichier.
Utilisez-la pour ranger la note ailleurs, par exemple dans vos Documents.

**À essayer**

1. Écrivez du texte dans une nouvelle note.
2. Faites `Ctrl+Maj+S`, choisissez votre dossier Documents et le nom
   `essai.txt`.
3. L'onglet s'appelle maintenant `essai.txt`. Modifiez le texte : un `*` apparaît
   devant le nom (« modifié, pas encore enregistré »). Faites `Ctrl+S` : l'étoile
   disparaît.

### Ouvrir un fichier

- **`Ctrl+O`** (ou l'icône **Ouvrir**) : choisissez un fichier. La fenêtre affiche
  par défaut les fichiers **texte (`.txt`) et Markdown (`.md`)** ; la liste
  déroulante des types permet de n'afficher que l'un des deux, ou de choisir
  **Tous les fichiers** pour voir les autres (`.py`, `.json`, `.markdown`...).
- **Glisser-déposer** : faites glisser un fichier depuis votre gestionnaire de
  fichiers jusque dans la fenêtre.
- Ouvrir un fichier **déjà ouvert** ne crée pas de doublon : l'application
  bascule simplement sur son onglet.
- Ouvrir de nouveau un fichier que vous aviez **fermé** réutilise sa note dans le
  panneau Brouillons : il n'y a toujours **qu'une seule entrée par fichier**. Si
  vous aviez laissé des modifications non enregistrées, vous les retrouvez (l'onglet
  a son `*`) ; sinon le contenu est relu depuis le disque.

### La coloration syntaxique

Selon l'extension du fichier, le texte est coloré automatiquement :

| Extension | Langage |
|---|---|
| `.py`, `.pyw` | Python |
| `.json` | JSON |
| `.md`, `.markdown` | Markdown (avec, en plus, un [aperçu](#aperçu-markdown)) |

Les autres fichiers restent en texte simple. La coloration se met à jour quand
vous changez l'extension avec **Enregistrer sous** (essayez de passer un texte de
`essai.txt` à `essai.py`).

---

## 7. Le panneau Brouillons

Le panneau de gauche liste **toutes vos notes archivées, ouvertes ou fermées**.
Les notes actuellement ouvertes sont suivies de « (ouvert) ». C'est votre filet de
sécurité : puisqu'on peut fermer un onglet sans confirmation, tout ce que vous
avez fermé se retrouve ici.

### Rouvrir une note

**Double-cliquez** sur son nom. Si elle est déjà ouverte, l'application bascule
simplement sur son onglet. La note de l'onglet actif est surlignée dans la liste.

### Rechercher et trier

- Le champ **Rechercher...** filtre la liste par nom pendant que vous tapez.
- La liste déroulante à côté trie par **Date** (les plus récentes d'abord) ou par
  **Nom**.

### Voir le chemin d'un fichier

Laissez la souris **un instant** sur une note du panneau (il faut compter environ
une seconde) : une petite infobulle affiche le **chemin complet du fichier**. C'est
pratique quand plusieurs fichiers portent le même nom dans des dossiers différents.
Pour une note qui n'est pas encore enregistrée dans un fichier, l'infobulle le dit
et indique où son brouillon est stocké. Les infobulles fonctionnent aussi quand la
fenêtre n'est pas au premier plan.

### Le menu contextuel

Un **clic droit sur une note** du panneau propose les mêmes actions que sur un
onglet (Fermer, Fermer les autres, Fermer à droite, Fermer tout, Épingler /
Détacher, Dupliquer, Historique des versions...), plus **Renommer...** et
**Mettre à la corbeille**. Les entrées de fermeture sont grisées pour une note
qui n'est pas ouverte.

Si un nom est trop long pour la largeur du panneau, faites défiler la liste
horizontalement, ou élargissez le panneau en déplaçant le séparateur.

**À essayer**

1. Fermez un onglet avec sa croix.
2. Repérez sa note dans le panneau (elle n'a plus « (ouvert) »).
3. Double-cliquez dessus : elle revient, avec son texte.

---

## 8. Épingler une note

Épingler une note **bloque sa fermeture et sa mise à la corbeille**. C'est utile
pour les notes que l'on veut toujours avoir sous la main et que l'on ne veut
surtout pas fermer par erreur.

### Épingler et détacher

Vous avez trois moyens, au choix :

- le **clic droit** sur l'onglet, ou sur la note dans le panneau Brouillons, puis
  **Épingler** (ou **Détacher** pour l'annuler) ;
- l'icône **Épingler** (punaise) de la barre d'outils, qui agit sur l'onglet
  actif ;
- l'icône **Détacher** (punaise barrée) pour l'annuler.

Une seule des deux icônes est active à la fois : « Épingler » est grisée si
l'onglet actif l'est déjà, « Détacher » est grisée s'il ne l'est pas. Les deux
sont grisées quand aucun onglet n'est ouvert.

### Ce que ça change

- Une **petite punaise** apparaît **à gauche du nom** de la note, dans l'onglet et
  dans le panneau Brouillons.
- L'onglet épinglé **n'a plus de croix** de fermeture.
- La fermeture est refusée par tous les moyens : `Ctrl+W`, l'icône Fermer
  l'onglet, l'entrée **Fermer** du menu. Un message dans la barre de statut vous
  explique pourquoi. **Mettre à la corbeille** est grisé.
- **Fermer les autres**, **Fermer à droite** et **Fermer tout** **ignorent** les
  notes épinglées : elles restent ouvertes, les autres sont fermées.
- **Quitter l'application n'est pas bloqué** : la note épinglée est restaurée,
  toujours épinglée, au lancement suivant.
- Vous pouvez encore renommer, dupliquer une note épinglée (la copie n'est pas
  épinglée) et consulter son historique. Vous pouvez aussi épingler une note
  **fermée** depuis le panneau Brouillons.

**À essayer**

1. Ouvrez trois onglets et épinglez le deuxième.
2. Faites un clic droit sur le premier → **Fermer tout**. Seul l'onglet épinglé
   reste.
3. Essayez `Ctrl+W` sur lui : rien ne se ferme, un message s'affiche.
4. Détachez-le : la croix revient, et vous pouvez le fermer.

---

## 9. La corbeille

Mettre une note à la corbeille est **réversible** : rien n'est détruit tant que
vous ne le demandez pas explicitement.

### Mettre une note à la corbeille

- Clic droit sur l'onglet, ou sur la note dans le panneau Brouillons, puis
  **Mettre à la corbeille**.
- L'application demande une **confirmation**.
- La note **quitte les onglets et le panneau Brouillons**.
- Une note **épinglée** ne peut pas être mise à la corbeille : détachez-la
  d'abord.

### Ouvrir la corbeille

Trois moyens : le bouton **Corbeille...** en bas du panneau Brouillons, l'icône
**Corbeille** de la barre d'outils, ou le menu **Fichier → Corbeille...**.

La fenêtre liste les notes avec leur date de suppression
(« … — supprimé le 19/09/2026 07:41 »).

### Restaurer ou supprimer définitivement

- **Restaurer** : la note retourne dans le panneau Brouillons (elle n'est pas
  rouverte automatiquement dans un onglet : double-cliquez dessus).
- **Supprimer définitivement** : efface la note pour de bon, après confirmation.
  **Cette action est irréversible.**

### Sélectionner plusieurs notes

Dans la fenêtre de la corbeille, vous pouvez agir sur plusieurs notes à la fois :

- **`Ctrl+clic`** ajoute ou retire une note de la sélection ;
- **`Maj+clic`** sélectionne toutes les notes entre deux clics ;
- **`Ctrl+A`** sélectionne tout.

**Restaurer** et **Supprimer définitivement** s'appliquent alors à **toute la
sélection**. Pour la suppression définitive, une **seule confirmation** est
demandée, qui indique le nombre de notes concernées.

**À essayer**

1. Créez trois notes et mettez-les à la corbeille.
2. Ouvrez la corbeille, sélectionnez-en deux avec `Ctrl+clic`, puis cliquez sur
   **Restaurer**. Elles reviennent dans le panneau Brouillons.

---

## 10. L'historique des versions

Chaque fois que vous **enregistrez un fichier déjà existant**, l'application
garde d'abord une copie de son **contenu précédent**. Elle conserve les **10
dernières versions** de chaque note ; les plus anciennes sont supprimées
automatiquement.

> Il faut donc au moins **deux enregistrements** d'une même note pour qu'une
> version apparaisse : la première fois, il n'y avait rien à sauvegarder.

### Consulter et restaurer

1. Clic droit sur l'onglet (ou sur la note dans le panneau Brouillons) →
   **Historique des versions...** (l'entrée n'apparaît, ou n'est active, que s'il
   existe des versions).
2. À gauche, la liste des versions par date et heure ; à droite, **l'aperçu** de
   la version sélectionnée.
3. **Restaurer cette version** remplace le texte de l'onglet par cette version.
   Le texte est repris comme « modifié » : vous pouvez le relire, puis
   l'enregistrer avec `Ctrl+S`.

> **Attention :** la restauration **ne peut pas être annulée avec `Ctrl+Z`**.
> Si vous tenez au texte actuel de la note, faites d'abord un clic droit →
> **Dupliquer** pour en garder une copie.

**À essayer**

1. Écrivez « version 1 » dans une note et faites `Ctrl+S`.
2. Remplacez par « version 2 » et refaites `Ctrl+S`.
3. Remplacez par « version 3 » et refaites `Ctrl+S`.
4. Ouvrez l'historique, sélectionnez la plus ancienne et restaurez-la.

---

## 11. Rechercher et remplacer

### Rechercher

Faites **`Ctrl+F`** (ou l'icône **Rechercher**) pour ouvrir la boîte de dialogue,
tapez le texte dans **Rechercher :**, puis :

- **Suivant** (ou **`F3`**) : va à l'occurrence suivante ;
- **Précédent** : va à l'occurrence précédente.

La recherche **reprend au début** (ou à la fin) quand elle atteint le bout du
texte. Si le texte n'existe pas, un message « … est introuvable » s'affiche.

Deux options affinent la recherche :

- **Sensible à la casse** : « Note » et « note » sont alors distincts ;
- **Mot entier** : « note » ne correspond plus à « notes » ni à « annoter ».

### Remplacer

Faites **`Ctrl+H`** (ou l'icône **Rechercher / Remplacer**). La boîte de dialogue
est la même, avec le champ **Remplacer par :** :

- **Remplacer** : remplace l'occurrence sélectionnée et passe à la suivante ;
- **Tout remplacer** : remplace toutes les occurrences d'un coup, puis affiche
  combien il y en avait (« 2 occurrence(s) remplacée(s). »).

`Tout remplacer` peut être annulé en une seule fois avec `Ctrl+Z`.

**À essayer**

1. Écrivez : « le chat dort, le chat mange ».
2. `Ctrl+H`, cherchez « chat », remplacez par « chien », cliquez sur **Tout
   remplacer**.
3. Appuyez sur `Ctrl+Z` pour tout annuler.

---

## 12. Confort d'écriture

### Retour automatique à la ligne

L'icône **Retour automatique à la ligne** (à droite de la barre d'outils) fait
passer les longues lignes à la ligne suivante au lieu de dépasser de la fenêtre.
Elle est **activée par défaut** ; cliquez dessus pour la désactiver. Le réglage
s'applique à **tous** les onglets, mais il est de nouveau activé à chaque
lancement.

### Aperçu Markdown

Pour un fichier Markdown (`.md` ou `.markdown`), l'icône **Aperçu Markdown** (la
dernière de la barre d'outils, une fenêtre coupée en deux) affiche à **droite de
l'éditeur** le texte tel qu'il apparaîtra une fois mis en forme : titres, gras,
italique, listes, cases à cocher, citations, code, liens, images.

- Le rendu se **met à jour en direct** pendant que vous tapez, et garde sa
  position de défilement.
- L'icône est **grisée** tant que l'onglet actif n'est pas un fichier Markdown. Une
  note qui n'est pas encore liée à un fichier n'en est pas un : faites d'abord
  **Enregistrer sous** avec un nom en `.md` (par exemple `journal.md`).
- Le volet suit l'onglet actif : il disparaît sur un onglet qui n'est pas Markdown
  et réapparaît quand vous revenez sur un onglet Markdown, tant que l'icône reste
  enfoncée. Recliquez sur l'icône pour le masquer.
- Les images et liens relatifs (`![](image.png)`) sont cherchés dans le dossier du
  fichier ; les liens vers le web s'ouvrent dans votre navigateur.
- Le volet est masqué à chaque lancement de l'application.

**À essayer**

1. Créez une note et écrivez `# Mon journal`, une ligne avec du `**gras**`, puis une
   liste avec des `- éléments`.
2. **Enregistrer sous** (`Ctrl+Maj+S`), nom `journal.md`. La coloration du texte
   apparaît et l'icône **Aperçu Markdown** devient active.
3. Cliquez dessus : le rendu s'affiche à droite. Modifiez le texte à gauche et
   regardez le rendu suivre.

### Numéros de ligne

Les numéros sont affichés à gauche de la zone d'édition ; la ligne du curseur est
surlignée. Ils suivent les lignes réelles du texte (une ligne qui passe à la ligne
suivante à l'écran garde un seul numéro).

### La barre de statut

En bas de la fenêtre, pour l'onglet actif :

- **Ligne X, Colonne Y** : la position du curseur ;
- **N mots, M caractères** : sur l'ensemble de la note ;
- **UTF-8** : l'encodage utilisé.

---

## 13. Fichier modifié par un autre programme

Si vous avez ouvert un fichier dans NoteEditor et qu'un **autre programme** le
modifie sur le disque, l'application le détecte quand vous **revenez sur l'onglet
ou sur la fenêtre**, puis vous demande si vous voulez **recharger** le contenu.

- **Oui** : le texte de l'onglet est remplacé par celui du disque. **Vos
  modifications non enregistrées dans cet onglet sont alors perdues.**
- **Non** : vous gardez votre version.

---

## 14. Lancer l'application au démarrage de la session

Pour que NoteEditor s'ouvre tout seul quand vous ouvrez votre session (et
restaure votre travail), un fichier `noteeditor.desktop` est fourni dans chacun
des dossiers `Python/` et `Cpp/`.

1. Choisissez **une seule** des deux versions.
2. Copiez son fichier dans le dossier des applications de démarrage :

   ```bash
   cp Python/noteeditor.desktop ~/.config/autostart/     # ou Cpp/noteeditor.desktop
   ```

3. Ouvrez ce fichier dans un éditeur : les lignes `Exec=` et `Path=` contiennent
   des **chemins absolus**, propres à la machine où le dépôt a été cloné. Adaptez-
   les si le vôtre est ailleurs.

Cela fonctionne avec LXQt (Lubuntu) et les autres bureaux qui suivent la
convention freedesktop. Sous LXQt, vous pouvez aussi passer par *Paramètres de
session → Démarrage automatique*.

---

## 15. Où sont mes données ?

Tout est rangé dans le dossier **`~/.noteeditor`** de votre répertoire personnel
(c'est un dossier caché : `Ctrl+H` dans la plupart des gestionnaires de fichiers
pour l'afficher).

| Élément | Contenu |
|---|---|
| `session.json` | Les onglets ouverts et l'onglet actif |
| `index.json` | La liste de toutes vos notes (nom, fichier associé, état épinglé) |
| `drafts/` | Le **texte** de chacune de vos notes, un fichier par note |
| `docs/` | Les fichiers créés par `Ctrl+S` sur une note sans fichier |
| `trash/` et `trash_index.json` | Les notes mises à la corbeille |
| `versions/` | Les 10 dernières versions de chaque fichier |
| `window.json` | La taille et la position de la fenêtre, et du séparateur |

**Sauvegarder vos notes** : copiez simplement le dossier `~/.noteeditor` sur une
clé USB ou un autre disque. Pour les retrouver sur une autre machine, copiez-le
dans le répertoire personnel de cette machine.

**Version Python ou C++, au choix** : les deux versions lisent et écrivent le même
dossier. Vous pouvez utiliser l'une aujourd'hui et l'autre demain, et retrouver
les mêmes notes.

> Ne modifiez pas les fichiers de ce dossier à la main pendant que l'application
> est ouverte.

---

## 16. Aide-mémoire des raccourcis

| Action | Raccourci |
|---|---|
| Nouvelle note (nouvel onglet) | `Ctrl+N` |
| Ouvrir un fichier | `Ctrl+O` |
| Enregistrer | `Ctrl+S` |
| Enregistrer sous | `Ctrl+Maj+S` |
| Fermer l'onglet | `Ctrl+W` |
| Quitter | `Ctrl+Q` |
| Annuler / Rétablir | `Ctrl+Z` / `Ctrl+Maj+Z` |
| Couper / Copier / Coller | `Ctrl+X` / `Ctrl+C` / `Ctrl+V` |
| Tout sélectionner | `Ctrl+A` |
| Rechercher | `Ctrl+F` |
| Rechercher / Remplacer | `Ctrl+H` |
| Occurrence suivante | `F3` |

Et à la souris :

| Geste | Effet |
|---|---|
| Double-clic sur une note du panneau Brouillons | La rouvre (ou bascule dessus) |
| Clic droit sur un onglet ou une note du panneau | Menu contextuel |
| Glisser un onglet | Réorganise les onglets |
| Glisser un fichier dans la fenêtre | L'ouvre dans un nouvel onglet |
| `Ctrl+clic` / `Maj+clic` dans la corbeille | Sélection multiple |

---

## 17. Questions fréquentes

**J'ai fermé un onglet par erreur, ma note est perdue ?**
Non. Regardez dans le panneau Brouillons : elle y est toujours. Double-cliquez
dessus pour la rouvrir.

**Je n'ai pas fait « Enregistrer », est-ce grave ?**
Non : la note est archivée automatiquement en continu et restaurée à la
réouverture. Vous n'avez besoin d'« Enregistrer » que si vous voulez un vrai
fichier, à l'endroit de votre choix.

**Je n'arrive pas à fermer un onglet.**
Il est probablement **épinglé** (une punaise à gauche du nom, pas de croix).
Faites un clic droit → **Détacher**, ou cliquez sur l'icône **Détacher**.

**« Mettre à la corbeille » est grisé.**
Même raison : la note est épinglée. Détachez-la d'abord.

**Où est passée ma note après « Mettre à la corbeille » ?**
Ouvrez la corbeille (bouton **Corbeille...** du panneau, ou icône de la barre
d'outils) et faites **Restaurer**.

**L'entrée « Historique des versions » est absente ou grisée.**
Aucune version n'existe encore pour cette note. Il faut avoir enregistré au moins
deux fois un fichier existant.

**« Renommer » n'apparaît pas pour ma note.**
On ne peut renommer que les notes qui ne sont pas liées à un fichier. Pour une
note liée à un fichier, renommez le fichier lui-même depuis votre gestionnaire de
fichiers, ou utilisez **Enregistrer sous**.

**Le même fichier apparaît plusieurs fois dans le panneau Brouillons.**
Cela venait d'anciennes versions de l'application, qui créaient une nouvelle entrée
à chaque ouverture d'un fichier fermé entre-temps. Ce n'est plus le cas, mais les
doublons déjà créés restent : mettez les entrées en trop à la corbeille (clic droit
→ **Mettre à la corbeille**). Ouvrir le fichier réutilise l'entrée la plus récente.

**Le nom d'une note est coupé dans le panneau Brouillons.**
Faites défiler le panneau horizontalement, ou élargissez-le en déplaçant le
séparateur. Le nom complet s'affiche aussi dans l'infobulle.

**J'ai deux versions de l'application, laquelle choisir ?**
Peu importe : elles font la même chose et partagent vos notes. Utilisez celle qui
est la plus simple à lancer chez vous.

**Pour aller plus loin :** la liste complète des fonctionnalités est dans
`FEATURES.md`, et les idées d'évolution dans `IMPROVEMENTS.md`.
