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
retrouve exactement son travail comme on l'avait laissé.** Il n'y a jamais de
question « Enregistrer les modifications ? » à la fermeture, et rien n'est perdu.

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

Ne sont pas restaurés : la position du curseur dans chaque onglet, l'historique
annuler/rétablir, et l'état de l'option de retour à la ligne (activée à chaque
lancement).

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
- Fermer un onglet (croix ou `Ctrl+W`) l'archive sans poser de question, même avec
  des modifications non enregistrées : il reste consultable dans le panneau
  Brouillons.

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
- **Menu contextuel** (clic droit sur un onglet) :
  - Fermer, Fermer les autres, Fermer à droite, Fermer tout
  - Dupliquer (nouvel onglet avec le même texte)
  - Renommer (pour les notes qui n'ont pas de fichier associé)
  - Historique des versions (quand des versions existent)
  - Mettre à la corbeille (après confirmation : la note quitte les onglets et le
    panneau Brouillons)

## 3. Édition

- **Numéros de ligne** dans la marge, avec **surlignage de la ligne courante**.
- **Coloration syntaxique** choisie automatiquement selon l'extension du fichier :
  Python (`.py`, `.pyw`), JSON (`.json`) et Markdown (`.md`, `.markdown`).
  Elle se met à jour quand l'extension change (Enregistrer sous). Les couleurs sont
  prévues pour un fond clair.
- **Retour automatique à la ligne**, activable et désactivable depuis la barre
  d'outils (activé par défaut, appliqué à tous les onglets).
- Annuler / Rétablir, Couper / Copier / Coller, Tout sélectionner (menu Édition).

## 4. Fichiers

- **Nouveau** (`Ctrl+N`), **Ouvrir** (`Ctrl+O`), **Enregistrer** (`Ctrl+S`),
  **Enregistrer sous** (`Ctrl+Maj+S`), **Fermer l'onglet** (`Ctrl+W`),
  **Quitter** (`Ctrl+Q`).
- **`Ctrl+S` sur une note sans fichier associé** l'enregistre directement dans
  `~/.noteeditor/docs/`, sous son nom par défaut, sans ouvrir de boîte de dialogue.
  `Ctrl+Maj+S` permet de choisir un autre emplacement.
- **Glisser-déposer** d'un fichier dans la fenêtre pour l'ouvrir dans un nouvel
  onglet.
- **Détection des modifications externes** : si un fichier ouvert est modifié sur
  le disque par un autre programme, l'application propose de le recharger lorsque
  l'on revient sur l'onglet ou sur la fenêtre. Recharger remplace le texte de
  l'onglet ; refuser le conserve.
- Ouvrir un fichier déjà ouvert bascule sur son onglet au lieu d'en créer un
  second.

## 5. Recherche et remplacement

- Boîte de dialogue de **Recherche** (`Ctrl+F`) et de **Recherche / Remplacement**
  (`Ctrl+H`).
- **Suivant** (`F3`) et **Précédent**, **Remplacer**, **Tout remplacer**.
- Options **Sensible à la casse** et **Mot entier**.

## 6. Panneau Brouillons

Panneau à gauche de la fenêtre qui liste **toutes les notes archivées dans
`~/.noteeditor`, ouvertes ou fermées**. Les notes actuellement ouvertes sont
marquées « (ouvert) ». C'est le filet de sécurité qui rend possible la fermeture
sans confirmation.

- **Double-clic** sur une note pour la rouvrir ; si elle est déjà ouverte, on
  bascule simplement sur son onglet.
- L'entrée de l'onglet actif est **surlignée** dans la liste.
- **Recherche** par nom et **tri** par date ou par nom.
- **Menu contextuel** (clic droit) : les mêmes actions que le menu des onglets
  (Fermer, Fermer les autres, Fermer à droite, Fermer tout, Dupliquer, Historique
  des versions) plus Renommer et Mettre à la corbeille. Les actions de fermeture
  sont grisées pour une note qui n'est pas ouverte.
- **Renommer** ne s'applique qu'aux notes sans fichier associé.
- Un brouillon n'est supprimé que manuellement, et seulement vers la corbeille.

## 7. Corbeille

- **Mettre à la corbeille** demande une confirmation, retire la note des onglets et
  du panneau Brouillons, et la déplace dans la corbeille sans la détruire.
- Bouton **Corbeille...** en bas du panneau Brouillons, icône de la barre d'outils
  et entrée du menu Fichier ouvrent la fenêtre de la corbeille.
- Depuis la corbeille : **Restaurer** une note (elle réapparaît dans les
  brouillons) ou la **supprimer définitivement**.

## 8. Historique des versions

- Chaque **enregistrement** d'un fichier archive d'abord son contenu précédent :
  les **10 dernières versions** sont conservées, les plus anciennes sont effacées.
- Accessible par clic droit sur l'onglet (ou sur la note dans le panneau
  Brouillons) → **Historique des versions...**, pour consulter une ancienne version
  et la **restaurer** dans l'onglet.

## 9. Barre d'outils

Icônes (dessinées par l'application, sans fichier d'image), de gauche à droite :
**Nouveau**, **Ouvrir**, **Enregistrer**, **Enregistrer sous**, **Fermer l'onglet**, **Corbeille**,
**Rechercher**, **Rechercher / Remplacer**, **Retour automatique à la ligne**.

## 10. Barre de statut

Pour l'onglet actif : **ligne et colonne** du curseur, **nombre de mots** et de
**caractères**, et **encodage** (UTF-8).

## 11. Menus

- **Fichier** : Nouveau, Ouvrir, Enregistrer, Enregistrer sous, Corbeille, Fermer
  l'onglet, Quitter.
- **Édition** : Annuler, Rétablir, Couper, Copier, Coller, Tout sélectionner.
- **Rechercher** : Rechercher, Rechercher / Remplacer, Suivant.
- **Aide** : À propos.

---

## Annexe : ce qui est stocké dans `~/.noteeditor`

| Élément | Rôle |
|---|---|
| `session.json` | Onglets ouverts et onglet actif (lu au lancement, réécrit à la fermeture) |
| `index.json` | Métadonnées de toutes les notes archivées (alimente le panneau Brouillons) |
| `drafts/` | Texte de chaque note, conservé même après la fermeture de son onglet |
| `docs/` | Fichiers réels créés par `Ctrl+S` depuis une note sans fichier associé |
| `trash/` et `trash_index.json` | Notes mises à la corbeille |
| `versions/<id>/` | Les 10 dernières versions de chaque fichier |
| `window.json` | Taille et position de la fenêtre, position du séparateur du panneau |

Chaque note est identifiée par un identifiant unique (UUID) : c'est lui, et non le
nom ou l'ordre des onglets, qui relie l'onglet, son texte archivé et son entrée
dans le panneau Brouillons.
