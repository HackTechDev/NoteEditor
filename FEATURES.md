# Fonctionnalités de NoteEditor

Liste complète des fonctionnalités de l'application. Elles sont identiques dans
les deux implémentations (`Python/` en PyQt6 et `Cpp/` en Qt6), qui partagent le
même format de données dans `~/.noteeditor` : on peut lancer l'une puis l'autre
sur la même machine et retrouver ses notes.

Les idées non encore réalisées sont dans `IMPROVEMENTS.md` ; le `README.md`
donne l'installation et le lancement.

---

## 1. Restauration de session (fonctionnalité principale)

L'idée centrale de NoteEditor : **on ferme l'application, on la rouvre, et on
retrouve exactement son travail comme on l'avait laissé.** Fermer l'application ne
pose jamais la question « Enregistrer les modifications ? », et rien n'est perdu.

### Ce qui est restauré au lancement suivant

- **Tous les onglets ouverts**, dans le même ordre (l'ordre est celui de la barre
  d'onglets, réorganisée par glisser-déposer si besoin).
- **Le contenu de chaque onglet**, y compris le texte **non enregistré** : une
  note jamais sauvegardée sur le disque revient avec son texte.
- **Le fichier associé** à chaque onglet (celui ouvert avec Ouvrir ou Enregistrer
  sous), et **l'état « modifié »** (l'astérisque `*` devant le nom de l'onglet).
- **L'onglet actif** : l'application se replace directement sur l'onglet où l'on
  travaillait.
- **La taille et la position de la fenêtre** sur l'écran. Si la position
  enregistrée n'est plus visible (par exemple un second écran débranché depuis),
  elle est ignorée et la fenêtre s'ouvre à l'emplacement par défaut.
- **La position du séparateur** entre le panneau Brouillons et la zone d'édition.
- **L'état « épinglé »** des notes (voir la section Onglets) : une note épinglée le
  reste d'un lancement à l'autre.
- **La position du curseur, la sélection et le défilement de chaque onglet** : on
  reprend exactement là où l'on travaillait dans chaque note, y compris dans les
  onglets qu'on n'avait pas encore réaffichés depuis le lancement.
- **L'historique annuler/rétablir de chaque onglet** : après un redémarrage, `Ctrl+Z`
  annule encore les dernières modifications faites avant la fermeture (jusqu'à 200
  étapes de chaque côté du point où l'on s'était arrêté), et `Ctrl+Maj+Z` les rétablit.
  Un historique qui ne correspond plus au texte de la note (par exemple parce qu'elle a
  été modifiée ailleurs) est ignoré ; pour une note très volumineuse (plus de 300 000
  caractères), l'historique n'est pas mémorisé. Il ne l'est qu'à la fermeture de
  l'application, pas à celle d'un onglet isolé.
- **Les réglages d'affichage** : le **retour automatique à la ligne** (activé ou non)
  et l'**aperçu Markdown** (activé ou non).

Ne sont pas restaurés : l'historique annuler/rétablir et la sélection de texte (seule
la position du curseur l'est).

### Comment c'est garanti

- **À la fermeture** de la fenêtre (croix, `Ctrl+Q`, menu Fichier → Quitter), la
  liste des onglets ouverts, l'onglet actif, la taille et la position de la
  fenêtre sont enregistrés, ainsi que le texte de chaque onglet.
- **Pendant le travail**, chaque onglet est aussi archivé automatiquement : dès sa
  création, **1,5 seconde après la dernière frappe**, et à sa fermeture. Un
  plantage ou une coupure de courant fait donc perdre au plus quelques secondes
  de frappe.
- Après un arrêt brutal (plantage, coupure), la liste des onglets rouverts est
  celle de la dernière fermeture normale, mais **le texte le plus récent de
  chaque note est conservé** et reste accessible dans le panneau Brouillons (voir
  plus bas), y compris pour les onglets ouverts depuis.
- La sauvegarde automatique ne touche **jamais** au fichier réel de l'utilisateur :
  elle écrit uniquement dans `~/.noteeditor`. Seul un « Enregistrer » explicite
  modifie un fichier ailleurs sur le disque.
- Fermer un onglet (croix ou `Ctrl+W`) d'une **note interne** (sans fichier
  associé, ou enregistrée sous `~/.noteeditor/docs/`) l'archive sans poser de
  question, même avec des modifications non enregistrées : elle reste consultable
  dans le panneau Brouillons.
- Un **fichier extérieur** à `~/.noteeditor` est traité à part (voir la section
  « Fichiers ») : son texte vit dans le fichier lui-même, il n'est plus listé une
  fois fermé, et une alerte s'affiche s'il a des modifications non enregistrées.

### Démarrage automatique au lancement de la session

Un fichier `noteeditor.desktop` est fourni dans `Python/` et dans `Cpp/`. Le
copier dans `~/.config/autostart/` (LXQt et autres bureaux freedesktop) lance
l'application à l'ouverture de session, qui restaure alors tout comme ci-dessus.
Ne copier qu'une des deux versions. Les chemins qu'il contient sont absolus et à
adapter si le dépôt est cloné ailleurs.

---

## 2. Onglets

- Onglets multiples, **réordonnables** par glisser-déposer, avec une **croix de
  fermeture** sur chacun ; l'onglet actif est bien visible (liseré bleu en haut, texte en
  gras).
- Bouton **+** pour créer un nouvel onglet, placé juste après le dernier onglet
  (comme Gedit). Quand les onglets débordent de la largeur disponible et que les
  flèches de défilement apparaissent, il se place à côté d'elles.
- Un **nouvel onglet est nommé par la date et l'heure** de sa création, au format
  `aammjj_hhmmssmm` (année, mois, jour, heure, minute, seconde, centièmes).
- Le titre de la fenêtre reprend le nom de l'onglet actif.
- **Infobulle au survol d'un onglet** : la même que dans le panneau Brouillons, c'est-à-dire
  le **chemin complet du fichier** (ou, pour une note sans fichier, l'emplacement de son
  brouillon). Pour une note épinglée, une ligne l'indique en plus. Elle se met à jour
  après un « Enregistrer sous », un renommage ou un changement d'état épinglé.
- **Menu contextuel** (clic droit sur un onglet) :
  - Fermer, Fermer les autres, Fermer à droite, Fermer tout
  - Dupliquer (nouvel onglet avec le même texte)
  - Renommer (pour les notes qui n'ont pas de fichier associé)
  - Historique des versions (quand des versions existent)
  - Épingler / Détacher (voir ci-dessous)
  - **Mémoriser à la fermeture** (case à cocher) : pour une note **sans texte** qui n'a
    pas de fichier, la fait figurer dans « Notes fermées récemment » quand on la ferme
    (elle n'y serait pas, sinon). L'état est retenu pour la note (d'un lancement à
    l'autre) ; il est sans effet pour une note qui contient du texte ou qui a un
    fichier, toujours mémorisée. La même entrée existe dans le menu du panneau
    Brouillons.
  - **Copier le nom du fichier** et **Copier le chemin complet du fichier** (dans le
    presse-papiers, prêts à coller) : le nom fonctionne pour toute note (nom du fichier,
    ou nom de la note si elle n'a pas de fichier), le chemin n'est disponible que pour
    une note liée à un fichier
  - **Ouvrir le dossier du fichier** : ouvre le dossier qui contient le fichier dans le
    gestionnaire de fichiers du bureau (grisé pour une note sans fichier ; un message
    s'affiche dans la barre de statut si le dossier n'existe plus)
  - Mettre à la corbeille (après confirmation : la note quitte les onglets et le
    panneau Brouillons)
- **Épingler une note** (clic droit → *Épingler*, à la fois sur l'onglet et sur la
  note dans le panneau Brouillons) **bloque sa fermeture et sa mise à la corbeille**,
  jusqu'à ce qu'on la *détache* (clic droit → *Détacher*).
  - Une **petite punaise** s'affiche à gauche du nom de la note : dans l'onglet (où
    la croix de fermeture disparaît, puisqu'on ne peut pas le fermer) et dans le
    panneau Brouillons, où tous les noms restent alignés.
  - Tout ce qui fermerait la note est bloqué : la croix, `Ctrl+W`, l'icône de la
    barre d'outils, « Fermer » du menu, et « Mettre à la corbeille ». Ces entrées de
    menu sont grisées. Un message dans la barre de statut explique le blocage.
  - Les fermetures en lot (« Fermer les autres », « Fermer à droite », « Fermer
    tout ») **ignorent** les notes épinglées et ferment les autres.
  - Les **onglets épinglés sont regroupés à gauche** de la barre d'onglets : épingler
    une note la place à la fin du groupe épinglé, la détacher la remet en tête des
    autres, et une note épinglée que l'on rouvre rejoint le groupe de gauche. Si l'on
    glisse un onglet de l'autre côté de la limite, il revient de son côté au
    relâchement de la souris. L'ordre est restauré tel quel au lancement suivant.
  - Quitter l'application n'est pas bloqué : une note épinglée est restaurée
    épinglée au lancement suivant. On peut aussi épingler une note fermée depuis le
    panneau Brouillons.
  - Deux icônes de la barre d'outils agissent sur l'onglet actif : **Épingler**
    (punaise) et **Détacher** (punaise barrée). Une seule est active à la fois :
    Épingler est grisée quand l'onglet est déjà épinglé, Détacher quand il ne
    l'est pas, et les deux sont grisées quand il n'y a aucun onglet.
  - Renommer, dupliquer (la copie n'est pas épinglée) et l'historique des versions
    restent possibles.

## 3. Édition

- **Numéros de ligne** dans la marge, avec **surlignage de la ligne courante**.
- **Coloration syntaxique** choisie automatiquement selon l'extension du fichier :
  Python (`.py`, `.pyw`), JSON (`.json`) et Markdown (`.md`, `.markdown`).
  Elle se met à jour quand l'extension change (Enregistrer sous). Les couleurs sont
  prévues pour un fond clair.
- **Retour automatique à la ligne**, activable et désactivable depuis la barre
  d'outils (activé par défaut, appliqué à tous les onglets, réglage mémorisé).
- **Aperçu Markdown** en volet séparé, à droite de l'éditeur, pour les fichiers
  `.md` et `.markdown` :
  - il s'active avec l'icône **Aperçu Markdown** de la barre d'outils, qui n'est
    disponible que pour un onglet Markdown (un fichier dont l'extension est `.md`
    ou `.markdown` ; une note sans fichier associé n'en est pas un tant qu'elle
    n'a pas été enregistrée sous un tel nom) ;
  - le rendu (titres, gras, italique, listes, cases à cocher, citations, code,
    liens, images) se **met à jour en direct** pendant la frappe, avec un court
    délai, et **conserve la position de défilement** ;
  - le volet suit l'onglet actif : il se masque sur un onglet qui n'est pas
    Markdown et revient sur un onglet Markdown, tant que l'icône est activée ;
  - les images et liens relatifs sont résolus depuis le dossier du fichier, et les
    liens externes s'ouvrent dans le navigateur ;
  - l'activation de l'aperçu est mémorisée d'un lancement à l'autre.
- Annuler (`Ctrl+Z`) / Rétablir (`Ctrl+Maj+Z`), Couper / Copier / Coller
  (`Ctrl+X` / `Ctrl+C` / `Ctrl+V`), Tout sélectionner (`Ctrl+A`), dans le menu Édition.

## 4. Fichiers

- **Nouveau** (`Ctrl+N`), **Ouvrir** (`Ctrl+O`, qui affiche par défaut les fichiers
  `.txt` et `.md`, avec des filtres séparés et « Tous les fichiers »), **Enregistrer** (`Ctrl+S`),
  **Enregistrer sous** (`Ctrl+Maj+S`), **Fermer l'onglet** (`Ctrl+W`),
  **Quitter** (`Ctrl+Q`).
- **`Ctrl+S` sur une note sans fichier associé** l'enregistre directement dans
  `~/.noteeditor/docs/`, sous son nom par défaut, sans ouvrir de boîte de dialogue.
  `Ctrl+Maj+S` permet de choisir un autre emplacement. La fenêtre propose les mêmes
  filtres que « Ouvrir » (texte et Markdown, texte, Markdown, tous les fichiers) et
  s'ouvre sur celui qui correspond au fichier courant.
- **Enregistrer sous vers un fichier qui a déjà une note** (fermée) reprend cette note
  au lieu d'en créer une seconde : **une seule entrée** dans le panneau Brouillons, son
  historique des versions et son état épinglé sont conservés et fusionnés avec ceux de
  la note enregistrée. Si le fichier est déjà ouvert dans un autre onglet, rien n'est
  fusionné.
- **Glisser-déposer** d'un fichier dans la fenêtre pour l'ouvrir dans un nouvel
  onglet.
- **Détection des modifications externes** : si un fichier ouvert est modifié sur
  le disque par un autre programme, l'application propose de le recharger lorsque
  l'on revient sur l'onglet ou sur la fenêtre. Recharger remplace le texte de
  l'onglet ; refuser le conserve.
- **Fermer un fichier extérieur** à `~/.noteeditor` (par la croix, `Ctrl+W`, ou
  **Fermer** depuis le panneau Brouillons) le **retire du panneau Brouillons** : son
  texte est dans le fichier lui-même, il n'a plus besoin d'y figurer. S'il a des
  **modifications non enregistrées**, une alerte propose **Enregistrer** (écrit le
  fichier puis ferme), **Ne pas enregistrer** (abandonne les modifications, le
  fichier n'est pas touché) ou **Annuler** (l'onglet reste ouvert). Les notes sans
  fichier et les fichiers sous `~/.noteeditor/docs/` restent dans le panneau. Quitter
  l'application n'affiche pas d'alerte : les modifications sont conservées et
  restaurées au lancement suivant. Rouvrir le fichier retrouve la même note
  (l'historique des versions continue).
- **Notes fermées récemment** (menu **Fichier → Notes fermées récemment**) : les 10
  dernières notes fermées, la plus récente en premier, à rouvrir d'un clic. Y
  figurent aussi les **fichiers extérieurs** à `~/.noteeditor`, qui n'apparaissent
  plus dans le panneau Brouillons une fois fermés : c'est le moyen le plus rapide de
  les retrouver. Une note fermée est mémorisée quelle que soit la façon de la fermer
  (croix, `Ctrl+W`, menus, « Fermer tout »...), sauf quitter l'application, ou une
  note mise à la corbeille (elle en est retirée). Une note **vide sans fichier** n'y
  est pas mémorisée par défaut (une note sans texte n'a rien à rouvrir) ; l'entrée
  **Mémoriser à la fermeture** du clic droit (voir ci-dessous) fait exception. Une
  note qui a été rouverte n'y figure plus. Un fichier extérieur est rouvert avec son
  **contenu actuel sur le disque**, et n'est plus proposé s'il a été supprimé.
  L'entrée **Effacer la liste** la vide ; l'infobulle d'une entrée donne le chemin
  du fichier. La liste est **mémorisée d'un lancement à l'autre**.
  - **Taille réglable** : la liste retient **10 notes par défaut** ; l'entrée **Nombre de
    notes mémorisées...** en bas du sous-menu permet de choisir de 1 à 50. Baisser le
    nombre raccourcit la liste tout de suite ; l'augmenter ne fait pas revenir les
    notes déjà oubliées. Le sous-menu reste accessible même quand la liste est vide
    (une ligne « Aucune note fermée récemment » grisée s'affiche alors).
- Ouvrir un fichier déjà ouvert bascule sur son onglet au lieu d'en créer un
  second.
- **Une seule entrée par fichier** dans le panneau Brouillons : rouvrir un fichier
  déjà ouvert puis fermé auparavant **réutilise son entrée** au lieu d'en créer une
  nouvelle (l'historique des versions suit). Si cette entrée avait des
  modifications non enregistrées, on les retrouve ; sinon le contenu est relu depuis
  le disque. Un double-clic sur une entrée dont le fichier est déjà ouvert dans un
  autre onglet bascule sur cet onglet.

## 5. Recherche et remplacement

- Boîte de dialogue de **Recherche** (`Ctrl+F`) et de **Recherche / Remplacement**
  (`Ctrl+H`).
- **Suivant** (`F3`) et **Précédent**, **Remplacer**, **Tout remplacer**.
- Options **Sensible à la casse** et **Mot entier**.

## 6. Panneau Brouillons

Panneau à gauche de la fenêtre qui liste **toutes les notes archivées dans
`~/.noteeditor`, ouvertes ou fermées**. Les notes actuellement ouvertes sont
marquées « (ouvert) ». C'est le filet de sécurité qui rend possible la fermeture
sans confirmation des notes internes. Exception : les **fichiers extérieurs à
`~/.noteeditor`** (ouverts avec Ouvrir) ne sont listés que tant qu'ils sont
ouverts.

- **Double-clic** sur une note pour la rouvrir ; si elle est déjà ouverte, on
  bascule simplement sur son onglet.
- **Sélection multiple** (`Ctrl+clic`, `Maj+clic`, `Ctrl+A`) : un clic droit sur la
  sélection propose des **actions groupées** :
  - **Fermer les N notes sélectionnées** : les notes épinglées sont ignorées, et si
    une note a des modifications non enregistrées et demande une alerte,
    **Annuler** arrête la fermeture des suivantes ;
  - **Épingler** / **Détacher les N notes sélectionnées** (l'entrée n'est active que
    s'il y a des notes à épingler, respectivement à détacher) ; la sélection est
    conservée après l'action ;
  - **Mettre les N notes sélectionnées à la corbeille** : une seule confirmation qui
    indique le nombre de notes, les notes épinglées sont ignorées (et signalées dans
    le message), et le texte le plus récent de chaque note ouverte part à la
    corbeille.
- L'entrée de l'onglet actif est **surlignée** dans la liste.
- **Recherche** par nom **ou par contenu** : le champ filtre les notes dont le nom
  *ou le texte* contient ce que l'on tape (sans tenir compte des majuscules et
  minuscules ; les accents, eux, doivent correspondre, comme pour les noms). Elle porte sur le texte archivé de chaque
  note ; les fichiers extérieurs fermés, qui ne sont plus listés, ne sont pas
  concernés. Et **tri** par date ou par nom.
- **Infobulle au survol** : laisser la souris sur une note affiche le **chemin
  complet de son fichier** ; pour une note pas encore enregistrée dans un fichier,
  l'infobulle indique où son brouillon est stocké. Les infobulles s'affichent même
  quand la fenêtre de l'application n'est pas au premier plan.
- **Menu contextuel** (clic droit) : les mêmes actions que le menu des onglets
  (Fermer, Fermer les autres, Fermer à droite, Fermer tout, Épingler / Détacher,
  Mémoriser à la fermeture, Dupliquer, Historique des versions) plus Renommer et
  Mettre à la corbeille. Les
  actions de fermeture sont grisées pour une note qui n'est pas ouverte, et
  « Fermer » comme « Mettre à la corbeille » le sont aussi pour une note épinglée.
- **Renommer** ne s'applique qu'aux notes sans fichier associé.
- **Copier le nom du fichier**, **Copier le chemin complet du fichier** et **Ouvrir le
  dossier du fichier** : les mêmes
  entrées que dans le menu des onglets, pour la note sur laquelle on a cliqué.
- Un brouillon n'est supprimé que manuellement, et seulement vers la corbeille.

## 7. Corbeille

- **Mettre à la corbeille** demande une confirmation, retire la note des onglets et
  du panneau Brouillons, et la déplace dans la corbeille sans la détruire.
- Bouton **Corbeille...** (avec son icône de poubelle) en bas du panneau Brouillons, icône de la barre d'outils
  et entrée du menu Fichier ouvrent la fenêtre de la corbeille.
- Depuis la corbeille : **Restaurer** une note (elle réapparaît dans les
  brouillons) ou la **supprimer définitivement**.
- **Sélection multiple** : `Ctrl+clic` ajoute ou retire une note, `Maj+clic`
  sélectionne une plage, `Ctrl+A` sélectionne tout. **Restaurer** et **Supprimer
  définitivement** s'appliquent alors à toute la sélection ; la suppression
  définitive ne demande qu'**une seule confirmation**, qui indique le nombre de
  brouillons concernés.

## 8. Historique des versions

- Chaque **enregistrement** d'un fichier archive d'abord son contenu précédent :
  les **10 dernières versions** sont conservées, les plus anciennes sont effacées.
- Accessible par clic droit sur l'onglet (ou sur la note dans le panneau
  Brouillons) → **Historique des versions...**, pour consulter une ancienne version
  et la **restaurer** dans l'onglet.

## 9. Barre d'outils

Icônes (dessinées par l'application, sans fichier d'image), de gauche à droite :
**Nouveau**, **Ouvrir**, **Enregistrer**, **Enregistrer sous**, **Fermer l'onglet**, **Épingler**, **Détacher**, **Corbeille**,
**Rechercher**, **Rechercher / Remplacer**, **Retour automatique à la ligne**, **Aperçu Markdown**.

## 10. Barre de statut

Pour l'onglet actif : **ligne et colonne** du curseur, **nombre de mots** et de
**caractères**, et **encodage** (UTF-8).

## 11. Menus

- **Fichier** : Nouveau, Ouvrir, Notes fermées récemment (sous-menu), Enregistrer,
  Enregistrer sous, Corbeille, Fermer l'onglet, Quitter.
- **Édition** : Annuler, Rétablir, Couper, Copier, Coller, Tout sélectionner.
- **Rechercher** : Rechercher, Rechercher / Remplacer, Suivant.
- **Aide** : À propos.

---

## Annexe : ce qui est stocké dans `~/.noteeditor`

| Élément | Rôle |
|---|---|
| `session.json` | Onglets ouverts, onglet actif, position du curseur, sélection et défilement de chaque onglet (lu au lancement, réécrit à la fermeture) |
| `index.json` | Métadonnées de toutes les notes archivées, dont l'état « épinglé » (alimente le panneau Brouillons) |
| `drafts/` | Texte de chaque note, conservé même après la fermeture de son onglet |
| `docs/` | Fichiers réels créés par `Ctrl+S` depuis une note sans fichier associé |
| `trash/` et `trash_index.json` | Notes mises à la corbeille |
| `versions/<id>/` | Les 10 dernières versions de chaque fichier |
| `history/<id>.json` | L'historique annuler/rétablir des onglets ouverts à la dernière fermeture de l'application |
| `recent.json` | Les dernières notes fermées et la taille de la liste (menu Fichier → Notes fermées récemment) |
| `window.json` | Taille et position de la fenêtre, position du séparateur du panneau, réglages d'affichage (retour à la ligne, aperçu Markdown) |

Chaque note est identifiée par un identifiant unique (UUID) : c'est lui, et non le
nom ou l'ordre des onglets, qui relie l'onglet, son texte archivé et son entrée
dans le panneau Brouillons.
