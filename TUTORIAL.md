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
   **Rechercher / Remplacer**, **Panneau Brouillons**, **Retour automatique à la
   ligne** et **Aperçu Markdown**. Laissez la
   souris un instant sur une icône : une infobulle donne son nom.
3. **Le panneau Brouillons** : la liste de toutes vos notes, ouvertes ou non
   (voir la [partie 7](#7-le-panneau-brouillons)).
4. **La barre d'onglets** : une note par onglet, avec le bouton **+** juste après
   le dernier.
5. **La zone d'édition** : avec les numéros de ligne à gauche, et la ligne où se
   trouve le curseur légèrement surlignée.
6. **La barre de statut** : le mode (insertion ou commande), la position du curseur,
   le nombre de mots et de caractères, et l'encodage.

Le séparateur entre le panneau et la zone d'édition se déplace à la souris. Sa
position, comme la taille et l'emplacement de la fenêtre, sera retenue.

---

## 3. Écrire votre première note

**À essayer**

1. Cliquez dans la zone d'édition et tapez quelques lignes.
2. Regardez la **barre de statut** : le mode (`-- INSERTION --`), la ligne, la
   colonne, le nombre de mots et de caractères changent en direct.
3. Regardez le **nom de l'onglet** : il ressemble à `260919_07352346`. C'est la
   date et l'heure de création de la note, au format `aammjj_hhmmssmm` (année,
   mois, jour, heure, minute, seconde, centièmes). Chaque nouvelle note reçoit
   ainsi un nom unique, sans que vous ayez à en choisir un.
4. Utilisez les raccourcis habituels : `Ctrl+Z` annuler, `Ctrl+Maj+Z` rétablir,
   `Ctrl+X` / `Ctrl+C` / `Ctrl+V` couper, copier, coller, `Ctrl+A` tout
   sélectionner (ils sont aussi dans le menu **Édition**).
5. Essayez le **mode commande**, inspiré de l'éditeur Vim : appuyez sur `Échap`. La
   barre de statut affiche `-- COMMANDE --` et le curseur devient un bloc. Dans ce mode
   la frappe n'insère pas de texte ; les flèches, elles, fonctionnent, et `Échap` de
   nouveau ramène au mode normal (`-- INSERTION --`). Quelques commandes :
   - `o` : insère **une ligne vide sous la ligne courante**, place le curseur dessus et
     vous ramène en mode insertion ;
   - `Maj+J` : **joint la ligne courante à la suivante** (elles sont séparées par une
     espace) ; vous restez en mode commande ;
   - `0` : le curseur va **au début de la ligne** ;
   - `$` : le curseur va **à la fin de la ligne**.
6. Pour **indenter** plusieurs lignes, sélectionnez-les puis appuyez sur `Tab` : chaque
   ligne est décalée de 4 espaces vers la droite. `Maj+Tab` fait l'inverse (retrait de
   4 espaces). Un seul `Ctrl+Z` annule toute l'opération. Sans sélection, `Tab` insère
   une tabulation comme d'habitude et `Maj+Tab` retire l'indentation de la ligne du
   curseur.

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
- l'**état « épinglé »** de vos notes (voir la [partie 8](#8-épingler-une-note)) ;
- **la position du curseur, la sélection et le défilement** de chaque note : vous
  reprenez là où vous vous étiez arrêté ;
- **l'historique annuler/rétablir** de chaque note : après un redémarrage, `Ctrl+Z`
  annule encore ce que vous aviez tapé avant de quitter ;
- **vos réglages d'affichage** : panneau Brouillons affiché ou masqué, retour
  automatique à la ligne et aperçu Markdown.

Si une note a des modifications non enregistrées dans son fichier, son nom
commence par une étoile `*` (par exemple `*rapport.txt`). Cette étoile est elle
aussi conservée.

Ce qui n'est pas retenu : l'historique annuler/rétablir, et la sélection de texte
(seule la position du curseur l'est).

**À essayer aussi : retrouver sa place.** Collez ou tapez un long texte (une centaine de
lignes), faites défiler jusqu'au milieu et cliquez sur une ligne, puis quittez et
relancez l'application : le curseur est sur la même ligne et le texte défilé au
même endroit, dans chacun de vos onglets.

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
- Pour une **note interne** (sans fichier, ou enregistrée dans `~/.noteeditor/docs/`),
  il n'y a **pas de confirmation** : fermer l'onglet ne détruit pas la note, elle
  reste dans le panneau Brouillons et vous pouvez la rouvrir quand vous voulez.
- Pour un **fichier extérieur** à `~/.noteeditor` (ouvert avec **Ouvrir**), la
  fermeture le **retire du panneau Brouillons** : son contenu est dans le fichier
  lui-même. S'il a des modifications non enregistrées, une alerte vous prévient
  qu'il faut l'enregistrer (voir la [partie 6](#fermer-un-fichier-extérieur)).
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
| **Mémoriser à la fermeture** | Case à cocher : une note vide sans fichier sera quand même proposée dans « Notes fermées récemment » |
| **Dupliquer** | Crée une nouvelle note avec le même texte |
| **Renommer...** | Change le nom (notes sans fichier associé) |
| **Historique des versions...** | Affiche les anciennes versions (partie 10) |
| **Copier le nom du fichier** | Copie dans le presse-papiers le nom du fichier (ou le nom de la note s'il n'y a pas de fichier) |
| **Copier le chemin complet du fichier** | Copie le chemin complet, nom compris (grisé pour une note sans fichier) |
| **Ouvrir le dossier du fichier** | Ouvre le dossier qui contient le fichier dans le gestionnaire de fichiers (grisé pour une note sans fichier) |
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
  par défaut avec l'extension `.md` (par exemple `260919_07352346.md`). Le nom
  de l'onglet devient alors celui du fichier.
- Sur une note **déjà liée à un fichier**, `Ctrl+S` réécrit ce fichier.

### Enregistrer sous (`Ctrl+Maj+S`)

Ouvre la fenêtre habituelle pour **choisir l'emplacement et le nom** du fichier.
Utilisez-la pour ranger la note ailleurs, par exemple dans vos Documents. Comme pour
**Ouvrir**, la liste des types de fichiers propose **texte et Markdown**, **texte**,
**Markdown** et **Tous les fichiers**, et s'ouvre sur celui qui correspond au fichier
en cours (par exemple « Fichiers Markdown » pour un `.md`).

Si vous enregistrez sous le nom d'un fichier dont vous aviez déjà une note (fermée),
l'application **reprend cette note** : il n'y a toujours qu'une seule entrée pour ce
fichier dans le panneau Brouillons, et son historique des versions est conservé.

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

### Rouvrir une note fermée récemment

Le menu **Fichier → Notes fermées récemment** liste les **dernières notes que vous avez
fermées** (10 par défaut), la plus récente en premier. Cliquez sur l'une d'elles pour la rouvrir.

- Cela fonctionne pour **toutes** vos notes, y compris les **fichiers extérieurs** à
  `~/.noteeditor`, qui ne figurent plus dans le panneau Brouillons une fois fermés :
  c'est le moyen le plus rapide de les retrouver. Un fichier est alors rouvert avec
  son contenu **actuel** sur le disque.
- Laissez la souris sur une entrée pour voir le **chemin** du fichier.
- Une note rouverte disparaît de la liste (elle est de nouveau ouverte), et une note
  mise à la corbeille aussi. Un fichier supprimé du disque n'est plus proposé.
- Une note **sans texte et sans fichier** n'y est pas mémorisée : elle n'aurait rien à
  vous rendre. Si vous voulez quand même la retrouver là, faites un **clic droit** sur
  son onglet (ou sur elle dans le panneau Brouillons) et cochez **Mémoriser à la
  fermeture** ; elle sera alors proposée quand vous la fermerez, même vide. Cette
  case n'a aucun effet pour une note qui contient du texte (toujours mémorisée).
- **Effacer la liste**, en bas du sous-menu, la vide.
- **Nombre de notes mémorisées...**, en bas du sous-menu, vous laisse choisir combien de
  notes garder, de 1 à 50 (10 par défaut). Baisser le nombre raccourcit la liste
  immédiatement. Ce réglage reste accessible même quand la liste est vide.
- La liste est **mémorisée** : vous la retrouvez au lancement suivant. Quitter
  l'application n'y ajoute rien (les onglets ouverts sont restaurés à part).

**À essayer :** créez une note, écrivez quelques mots, fermez son onglet, puis rouvrez-la
par **Fichier → Notes fermées récemment**.

### Fermer un fichier extérieur

Un fichier que vous avez ouvert depuis **un autre dossier que `~/.noteeditor`**
(vos Documents, un projet...) est traité différemment d'une simple note : son
contenu est **dans le fichier lui-même**, donc l'application ne le garde pas dans le
panneau Brouillons une fois fermé.

- **Fermer** un tel fichier (croix, `Ctrl+W`, ou **Fermer** dans le panneau
  Brouillons) le **retire du panneau**.
- S'il a des **modifications non enregistrées**, une alerte vous prévient qu'il faut
  l'enregistrer, avec trois choix :
  - **Enregistrer** : écrit le fichier, puis ferme ;
  - **Ne pas enregistrer** : ferme en abandonnant les modifications (le fichier sur
    le disque n'est pas touché) ;
  - **Annuler** : l'onglet reste ouvert.
- **Quitter l'application** n'affiche pas cette alerte : les modifications sont
  conservées et vous les retrouvez au prochain lancement.
- Pour le rouvrir, utilisez **Ouvrir** (`Ctrl+O`). Vous retrouvez la même note, avec
  son historique des versions.
- Les notes **sans fichier** et les fichiers enregistrés dans `~/.noteeditor/docs/`
  (par `Ctrl+S` sur une nouvelle note) ne sont pas concernés : ils restent dans le
  panneau.

**À essayer**

1. Ouvrez un fichier de vos Documents avec `Ctrl+O` : il apparaît dans le panneau.
2. Tapez quelques caractères, puis fermez l'onglet : l'alerte s'affiche.
3. Choisissez **Enregistrer** : l'onglet se ferme et le fichier disparaît du panneau.

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
sécurité : puisqu'on peut fermer une note interne sans confirmation, tout ce que
vous avez fermé se retrouve ici. Les **fichiers extérieurs** à `~/.noteeditor` font
exception : ils ne figurent dans la liste que tant qu'ils sont ouverts.

### Afficher ou masquer le panneau

Pour gagner de la place, l'icône **Panneau Brouillons** de la barre d'outils (une
fenêtre dont la colonne de gauche est pleine, à gauche de l'icône de retour à la
ligne) **masque** le panneau, et le rend en un second clic. L'icône est enfoncée tant
que le panneau est visible. Le panneau retrouve sa largeur d'avant, et ce choix est
**mémorisé** d'un lancement à l'autre. Les notes ne sont pas affectées : les onglets
restent ouverts, et la corbeille reste accessible par la barre d'outils et le menu
**Fichier**.

### Rouvrir une note

**Double-cliquez** sur son nom. Si elle est déjà ouverte, l'application bascule
simplement sur son onglet. La note de l'onglet actif est surlignée dans la liste.

### Rechercher et trier

- Le champ **Rechercher...** filtre la liste pendant que vous tapez, **par nom ou par
  contenu** : une note apparaît si son nom *ou son texte* contient ce que vous avez
  saisi. Tapez un mot dont vous vous souvenez pour retrouver une note dont vous ne
  vous rappelez plus le nom. (Les fichiers extérieurs à `~/.noteeditor` que vous avez
  fermés ne sont pas listés, donc pas cherchés.)
- La liste déroulante à côté trie par **Date** (les plus récentes d'abord) ou par
  **Nom**.

### Voir le chemin d'un fichier

Laissez la souris **un instant** sur une note du panneau **ou sur son onglet** (il
faut compter environ une seconde) : une petite infobulle affiche le **chemin complet
du fichier**. C'est pratique quand plusieurs fichiers portent le même nom dans des
dossiers différents. Pour une note qui n'est pas encore enregistrée dans un fichier,
l'infobulle le dit et indique où son brouillon est stocké. Les infobulles
fonctionnent aussi quand la fenêtre n'est pas au premier plan.

Pour **réutiliser ce chemin ailleurs** (un terminal, un message, un autre programme),
faites un **clic droit** sur la note, dans le panneau ou sur son onglet, puis :

- **Copier le nom du fichier** : place le nom (par exemple `rapport.md`) dans le
  presse-papiers ;
- **Copier le chemin complet du fichier** : place le chemin complet, nom compris
  (par exemple `/home/vous/Documents/rapport.md`). Cette entrée est grisée pour une
  note qui n'est pas liée à un fichier ;
- **Ouvrir le dossier du fichier** : ouvre le dossier du fichier dans le gestionnaire de
  fichiers du bureau (grisé lui aussi sans fichier).

Collez ensuite avec `Ctrl+V`.

### Agir sur plusieurs notes d'un coup

Sélectionnez plusieurs notes avec **`Ctrl+clic`** (une par une), **`Maj+clic`**
(une plage) ou **`Ctrl+A`** (toutes), puis faites un **clic droit** sur la sélection.
Le menu propose des actions **pour toute la sélection** :

- **Fermer les N notes sélectionnées** : les notes épinglées restent ouvertes. Si une
  alerte s'affiche pour un fichier modifié et que vous cliquez sur **Annuler**, la
  fermeture des notes suivantes est interrompue.
- **Épingler les N notes sélectionnées** et **Détacher les N notes sélectionnées** :
  chacune n'est active que s'il y a quelque chose à faire (des notes non épinglées,
  respectivement des notes épinglées). La sélection reste en place, vous pouvez donc
  enchaîner une autre action.
- **Mettre les N notes sélectionnées à la corbeille** : une seule confirmation, qui
  indique combien de notes sont concernées. Les notes épinglées sont **ignorées**
  (le message le précise) : détachez-les d'abord si vous voulez aussi les mettre à
  la corbeille.

**À essayer :** créez quatre notes, épinglez-en une, sélectionnez les quatre dans le
panneau (`Ctrl+A`), puis choisissez **Mettre les 4 notes sélectionnées à la
corbeille** : trois partent à la corbeille, l'épinglée reste.

### Le menu contextuel

Un **clic droit sur une note** du panneau propose les mêmes actions que sur un
onglet (Fermer, Fermer les autres, Fermer à droite, Fermer tout, Épingler /
Détacher, Mémoriser à la fermeture, Dupliquer, Historique des versions..., Copier le
nom du fichier, Copier le chemin complet du fichier, Ouvrir le dossier du fichier), plus **Renommer...** et
**Mettre à la corbeille**. Les entrées de fermeture sont grisées pour une note qui
n'est pas ouverte.

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
- Les **onglets épinglés se regroupent à gauche** de la barre d'onglets, avant les
  autres. Si vous faites glisser un onglet de l'autre côté de la limite, il revient
  de son côté quand vous relâchez la souris.
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
s'applique à **tous** les onglets et il est **mémorisé** d'un lancement à l'autre.

### Aperçu Markdown

Pour un fichier Markdown (`.md` ou `.markdown`), l'icône **Aperçu Markdown** (la
dernière de la barre d'outils, une fenêtre coupée en deux) affiche à **droite de
l'éditeur** le texte tel qu'il apparaîtra une fois mis en forme : titres, gras,
italique, listes, cases à cocher, citations, code, liens, images.

- Le rendu se **met à jour en direct** pendant que vous tapez, et garde sa
  position de défilement.
- Le volet **suit le défilement de l'éditeur** : quand vous faites défiler le texte à
  gauche (molette, barre de défilement, curseur qui descend), le rendu à droite
  défile en même temps, au même endroit relatif.
- L'icône est **grisée** tant que l'onglet actif n'est pas un fichier Markdown. Une
  note qui n'est pas encore liée à un fichier n'en est pas un : faites d'abord
  `Ctrl+S`, qui l'enregistre en `.md`, ou **Enregistrer sous** avec un nom en `.md`.
- Le volet suit l'onglet actif : il disparaît sur un onglet qui n'est pas Markdown
  et réapparaît quand vous revenez sur un onglet Markdown, tant que l'icône reste
  enfoncée. Recliquez sur l'icône pour le masquer.
- Les images et liens relatifs (`![](image.png)`) sont cherchés dans le dossier du
  fichier ; les liens vers le web s'ouvrent dans votre navigateur.
- Le fait d'avoir activé l'aperçu est mémorisé : au lancement suivant, le volet
  réapparaît dès qu'un onglet Markdown est affiché.

**À essayer**

1. Créez une note et écrivez `# Mon journal`, une ligne avec du `**gras**`, puis une
   liste avec des `- éléments`.
2. Faites `Ctrl+S` (la note est enregistrée en `.md`) ou **Enregistrer sous**
   (`Ctrl+Maj+S`) avec le nom `journal.md`. La coloration du texte apparaît et
   l'icône **Aperçu Markdown** devient active.
3. Cliquez dessus : le rendu s'affiche à droite. Modifiez le texte à gauche et
   regardez le rendu suivre.

### Exporter en HTML

Pour partager le rendu ou le mettre en ligne, choisissez **Fichier → Exporter en
HTML...** : la note Markdown active est enregistrée sous forme de **page HTML**
autonome (encodage UTF-8, titre = nom du fichier). L'entrée est grisée pour une note
qui n'est pas un fichier Markdown ; l'aperçu n'a pas besoin d'être affiché.

- La boîte de dialogue propose le **dossier du fichier Markdown** et le nom
  `note.html` (le nom de la note avec l'extension `.html`). Gardez ce dossier si la
  note contient des **images** ou des liens relatifs : ils sont conservés tels quels
  dans la page et ne s'affichent que si elle est à côté d'eux.
- L'export part du **texte actuel**, y compris les modifications pas encore
  enregistrées dans le fichier `.md`.
- Une fois l'export fait, un message dans la barre de statut donne le fichier créé.

### Numéros de ligne

Les numéros sont affichés à gauche de la zone d'édition ; la ligne du curseur est
surlignée. Ils suivent les lignes réelles du texte (une ligne qui passe à la ligne
suivante à l'écran garde un seul numéro).

### La barre de statut

En bas de la fenêtre, pour l'onglet actif :

- **-- INSERTION -- / -- COMMANDE --** : le mode de l'éditeur (voir la partie 3) ;
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
| `session.json` | Les onglets ouverts, l'onglet actif, et le curseur, la sélection et le défilement de chacun |
| `index.json` | La liste de toutes vos notes (nom, fichier associé, état épinglé) |
| `drafts/` | Le **texte** de chacune de vos notes, un fichier par note |
| `docs/` | Les fichiers créés par `Ctrl+S` sur une note sans fichier |
| `trash/` et `trash_index.json` | Les notes mises à la corbeille |
| `versions/` | Les 10 dernières versions de chaque fichier |
| `history/` | L'historique annuler/rétablir des notes ouvertes à la dernière fermeture |
| `recent.json` | Les dernières notes fermées et la taille de la liste (menu Fichier) |
| `window.json` | La taille et la position de la fenêtre, du séparateur, et vos réglages d'affichage |

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
| Indenter les lignes sélectionnées | `Tab` |
| Retirer une indentation | `Maj+Tab` |
| Mode commande (activer / quitter) | `Échap` |
| Insérer une ligne dessous (mode commande) | `o` |
| Joindre la ligne à la suivante (mode commande) | `Maj+J` |
| Début de ligne (mode commande) | `0` |
| Fin de ligne (mode commande) | `$` |
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
Non, pour une note interne : regardez dans le panneau Brouillons, elle y est
toujours. Double-cliquez dessus pour la rouvrir. Toute note fermée, fichier extérieur
compris, se retrouve aussi dans **Fichier → Notes fermées récemment**. Pour un fichier extérieur à
`~/.noteeditor`, il n'est plus listé une fois fermé, mais son contenu est dans le
fichier : rouvrez-le avec **Ouvrir** (`Ctrl+O`).

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

**Mon fichier n'apparaît plus dans le panneau Brouillons après l'avoir fermé.**
C'est voulu pour un fichier **extérieur à `~/.noteeditor`** (ouvert avec **Ouvrir**) :
son contenu est dans le fichier lui-même, il n'est donc listé que tant qu'il est
ouvert. Rouvrez-le avec **Ouvrir** (`Ctrl+O`) ; vous retrouvez la même note et son
historique des versions. Les notes sans fichier et celles enregistrées dans
`~/.noteeditor/docs/` restent, elles, toujours dans le panneau.

**Une alerte « Modifications non enregistrées » s'affiche quand je ferme un onglet.**
Elle concerne un fichier extérieur à `~/.noteeditor` qui a été modifié sans être
enregistré : **Enregistrer** écrit le fichier puis ferme, **Ne pas enregistrer**
abandonne les modifications, **Annuler** garde l'onglet ouvert. Elle ne s'affiche pas
quand vous quittez l'application (les modifications sont alors conservées et
restaurées au lancement suivant).

**L'icône « Aperçu Markdown » est grisée.**
L'onglet actif n'est pas un fichier Markdown. L'aperçu n'est disponible que pour un
fichier dont l'extension est `.md` ou `.markdown` ; une note qui n'est pas encore liée
à un fichier n'en est pas un. Faites `Ctrl+S` (qui l'enregistre en `.md`) ou **Enregistrer sous**
avec un nom en `.md`.

**Comment récupérer le chemin d'un fichier pour le coller ailleurs ?**
Clic droit sur la note (dans le panneau ou sur son onglet) → **Copier le chemin
complet du fichier**, puis `Ctrl+V` là où vous en avez besoin. **Copier le nom du
fichier** ne copie que le nom.

**Ma note vide n'apparaît pas dans « Notes fermées récemment ».**
C'est voulu : une note **sans texte et sans fichier** n'a rien à rouvrir, elle n'est
donc pas mémorisée par défaut. Pour la retrouver quand même, faites un clic droit sur
son onglet (ou sur elle dans le panneau Brouillons) et cochez **Mémoriser à la
fermeture** avant de la fermer.

**La recherche du panneau ne trouve pas un mot que je sais avoir écrit.**
Vérifiez d'abord les **accents** : la recherche ignore les majuscules mais pas les
accents, donc « zebulon » ne trouve pas « zébulon ». Elle ne porte aussi que sur les
notes **listées** dans le panneau : un fichier extérieur à `~/.noteeditor` que vous avez
fermé n'y figure plus, donc n'est pas cherché (rouvrez-le par **Fichier → Notes
fermées récemment** ou **Ouvrir**).

**J'ai deux versions de l'application, laquelle choisir ?**
Peu importe : elles font la même chose et partagent vos notes. Utilisez celle qui
est la plus simple à lancer chez vous.

**Pour aller plus loin :** la liste complète des fonctionnalités est dans
`FEATURES.md`, et les idées d'évolution dans `IMPROVEMENTS.md`.
