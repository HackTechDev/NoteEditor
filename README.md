# NoteEditor

Éditeur de texte à onglets. Deux implémentations équivalentes cohabitent dans ce dépôt et partagent le même format de données (`~/.noteeditor`) :

- **`Python/`** — Python + PyQt6
- **`Cpp/`** — C++ + Qt6

## Fonctionnalités

La liste détaillée et complète est dans [`FEATURES.md`](FEATURES.md), et un didacticiel pas à pas pour apprendre à utiliser le logiciel dans [`TUTORIAL.md`](TUTORIAL.md). En résumé :

- Session persistante : à la fermeture, tous les onglets (contenu, fichier associé, état modifié, onglet actif) sont sauvegardés automatiquement dans `~/.noteeditor` et restaurés tels quels au prochain lancement, ainsi que la taille et la position de la fenêtre sur l'écran (ignorée si elle n'est plus visible, p. ex. écran débranché) et la position du séparateur du panneau latéral
- Chaque onglet est archivé dans `~/.noteeditor` dès sa création, et à nouveau en continu pendant la frappe (1,5s après la dernière touche), à sa fermeture (croix ou Ctrl+W, sans confirmation même en cas de modifications non enregistrées) et à la fermeture de l'appli
- Onglets multiples (fermables, réordonnables), onglet actif bien visible
- Bouton **+** pour créer un nouvel onglet, collé juste après le dernier onglet (style Gedit) ; se déplace automatiquement à côté des flèches de défilement quand les onglets débordent de la largeur disponible
- Nouveaux onglets nommés par date/heure (`aammjj_hhmmssmm`)
- Menu contextuel sur les onglets (clic droit) : fermer / fermer les autres / fermer à droite / fermer tout, dupliquer, renommer, historique des versions, mettre à la corbeille (l'onglet se ferme et la note quitte le panneau Brouillons), épingler / détacher
- Notes épinglées : « Épingler » (menu contextuel d'un onglet ou du panneau Brouillons) bloque la fermeture et la mise à la corbeille de la note, jusqu'à « Détacher » ; une petite punaise s'affiche à gauche de son nom (dans l'onglet, où la croix de fermeture disparaît, et dans le panneau), et l'état est restauré avec la session
- Numéros de ligne avec surlignage de la ligne courante
- Coloration syntaxique automatique selon l'extension : Python (`.py`), JSON (`.json`), Markdown (`.md`)
- Recherche / remplacement (`Ctrl+F` / `Ctrl+H`) : suivant, précédent, remplacer, tout remplacer
- `Ctrl+S` sur un onglet sans fichier associé l'enregistre directement dans `~/.noteeditor/docs/` (sous son nom par défaut), sans ouvrir de boîte de dialogue ; `Ctrl+Shift+S` (Enregistrer sous) permet de choisir un autre emplacement
- Glisser-déposer un fichier dans la fenêtre pour l'ouvrir dans un nouvel onglet
- Détection de modification externe : si le fichier ouvert change sur le disque (autre programme), l'appli propose de recharger
- Panneau « Brouillons » à gauche : liste tous les onglets archivés dans `~/.noteeditor` (ouverts marqués « (ouvert) »), avec recherche, tri (date/nom) et l'entrée de l'onglet actif surlignée ; double-clic pour rouvrir ou basculer dessus, clic droit pour la même palette d'actions que le menu contextuel des onglets (fermer / fermer les autres / fermer à droite / fermer tout, dupliquer, historique des versions), renommer (onglets sans fichier) ou mettre à la corbeille
- Corbeille : la suppression d'un brouillon est réversible (bouton « Corbeille... » avec son icône en bas du panneau Brouillons, ou menu Fichier), avec restauration ou suppression définitive, y compris de plusieurs brouillons à la fois (sélection multiple)
- Historique des versions (10 dernières) : chaque enregistrement archive le contenu précédent du fichier, consultable et restaurable depuis le menu contextuel d'un onglet (clic droit)
- Barre d'outils avec icônes Nouveau, Ouvrir, Enregistrer, Enregistrer sous, Fermer l'onglet, Épingler, Détacher, Corbeille, Rechercher, Rechercher / Remplacer et retour à la ligne automatique (ce dernier activable/désactivable, actif par défaut, s'applique à tous les onglets)
- Barre de statut : position ligne/colonne, nombre de mots/caractères et encodage (UTF-8) de l'onglet actif
- Menu Aide → À propos

## Installation et lancement

### Python

```bash
cd Python
pip install -r requirements.txt
python3 main.py
```

### C++

Nécessite `qt6-base-dev` (fournit les en-têtes et `Qt6Config.cmake`) en plus d'un compilateur C++17 et de CMake.

```bash
cd Cpp
cmake -B build
cmake --build build -j"$(nproc)"
./build/NoteEditor
```

## Raccourcis clavier

| Action                  | Raccourci   |
|--------------------------|-------------|
| Nouvel onglet            | Ctrl+N      |
| Ouvrir                   | Ctrl+O      |
| Enregistrer               | Ctrl+S      |
| Enregistrer sous          | Ctrl+Shift+S|
| Fermer l'onglet          | Ctrl+W      |
| Quitter                  | Ctrl+Q      |
| Rechercher               | Ctrl+F      |
| Rechercher / Remplacer   | Ctrl+H      |
| Suivant                  | F3          |

## Structure du projet

| Fichier                        | Rôle                                                                     |
|----------------------------------|-----------------------------------------------------------------------------|
| `Python/main.py` / `Cpp/MainWindow.*` | Fenêtre principale, gestion des onglets, menus, ouverture/enregistrement |
| `Python/editor_widget.py` / `Cpp/Editor.*` | Widget d'édition (gouttière de numéros de ligne, ligne courante)   |
| `Python/highlighters.py` / `Cpp/Highlighters.*` | Coloration syntaxique (Python, JSON, Markdown)                |
| `Python/find_replace.py` / `Cpp/FindReplaceDialog.*` | Boîte de dialogue de recherche / remplacement           |
| `Python/session.py` / `Cpp/Session.*` | Sauvegarde et restauration de la session dans `~/.noteeditor`           |
| `Python/drafts_browser.py` / `Cpp/DraftsBrowser.*` | Panneau latéral listant les brouillons (ouverts et fermés)  |
| `Python/trash_dialog.py` / `Cpp/TrashDialog.*` | Boîte de dialogue de la corbeille (restaurer / supprimer définitivement) |
| `Python/version_history_dialog.py` / `Cpp/VersionHistoryDialog.*` | Historique des versions d'un onglet     |

Les deux implémentations lisent/écrivent exactement le même format dans `~/.noteeditor` : on peut lancer l'une puis l'autre indifféremment sur la même machine, elles se partagent les onglets ouverts et les brouillons.

## Session (`~/.noteeditor`)

- `~/.noteeditor/session.json` : liste des onglets actuellement ouverts (fichier associé, nom par défaut, état modifié, onglet actif)
- `~/.noteeditor/index.json` : métadonnées de tous les brouillons jamais sauvegardés (pour l'affichage dans le panneau « Brouillons »)
- `~/.noteeditor/drafts/` : contenu de chaque onglet, conservé même après la fermeture de son onglet
- `~/.noteeditor/docs/` : fichiers réels créés par `Ctrl+S` depuis un onglet sans titre
- `~/.noteeditor/trash/` et `trash_index.json` : brouillons mis à la corbeille (suppression réversible)
- `~/.noteeditor/versions/<id>/` : les 10 dernières versions d'un fichier avant chaque écrasement par un enregistrement
- `~/.noteeditor/window.json` : taille et position de la fenêtre, position du séparateur du panneau latéral, restaurées au lancement suivant

Cette copie de secours n'écrase jamais le fichier d'origine sur le disque : seul un `Enregistrer` explicite (`Ctrl+S`) modifie le fichier réel (que ce soit dans `~/.noteeditor/docs/` pour un onglet sans titre, ou à l'emplacement d'origine pour un fichier ouvert ailleurs). Les brouillons ne sont supprimés que manuellement, depuis le panneau latéral (et ne le sont alors que déplacés vers la corbeille).
