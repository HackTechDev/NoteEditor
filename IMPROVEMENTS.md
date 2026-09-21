# Idées d'amélioration

Liste de pistes pour NoteEditor, classées par thème. Rien n'est engagé,
c'est juste un réservoir d'idées à picorer. Les fonctionnalités déjà
implémentées sont documentées dans `README.md` et `FEATURES.md`, pas ici.

## Édition

- **Police et taille configurables** : actuellement fixées en dur (Monospace 11) dans `editor_widget.py` et `Editor.cpp`. Un réglage (menu ou raccourci `Ctrl+molette`) rendrait l'appli plus confortable selon l'écran.
- **Plus de commandes dans le mode commande** : seuls `o` et `Maj+J` existent aujourd'hui ; `O` (ligne au-dessus), `h`/`j`/`k`/`l`, `x`, `dd`, `yy`/`p`, `u`, `gg`/`G`, `/` pour chercher, voire `:w` / `:q`, en feraient un vrai mode Vim. Les touches non gérées sont simplement ignorées.
- **Réglage de l'indentation** : `Tab` / `Maj+Tab` décalent de 4 espaces en dur ; une largeur configurable, ou le choix entre espaces et tabulation, s'accorderait à l'usage de chacun (et à celui de chaque langage).
- **Indentation automatique** et **correspondance des parenthèses/accolades** pour les fichiers de code.
- **Plus de langages** pour la coloration syntaxique (`highlighters.py` / `Highlighters.cpp` ne couvrent que Python/JSON/Markdown) : JS, HTML, CSS, YAML, Shell seraient des ajouts naturels.
- **Aperçu Markdown** : synchroniser son défilement avec celui de l'éditeur, et pouvoir exporter le rendu en HTML.
- **Correcteur orthographique** (via `pyspellchecker` ou l'intégration d'un dictionnaire système).

## Interface

- **Thème sombre**, avec bascule manuelle ou suivi du thème système — les couleurs sont actuellement pensées uniquement pour un fond clair (`highlighters.py`, styles des onglets, aperçu Markdown).
- **Raccourci pour rouvrir la dernière note fermée** (par exemple `Ctrl+Maj+T`, comme dans un navigateur) : les notes fermées récemment sont déjà listées dans le menu Fichier, un raccourci les rendrait accessibles sans la souris.
- **Palette de commandes** (`Ctrl+Maj+P`) pour retrouver rapidement une action sans fouiller les menus.

## Panneau Brouillons

- **Recherche plus tolérante** : elle distingue aujourd'hui les accents (« zebulon » ne trouve pas « zébulon »), ne cherche que dans le texte archivé et ne montre pas où le mot a été trouvé ; ignorer les accents et afficher un extrait de la ligne correspondante l'améliorerait.

## Session et fichiers

- **Historique annuler/rétablir à la fermeture d'un onglet** : il n'est mémorisé qu'à la fermeture de l'application ; rouvrir une note fermée depuis « Notes fermées récemment » ou le panneau Brouillons repart d'une pile vide. Le conserver aussi dans ce cas demanderait de purger ces fichiers avec les brouillons.
- **Extension par défaut réglable** : `Ctrl+S` enregistre désormais en `.md` ; un réglage (`.md` ou `.txt`) conviendrait à qui ne prend pas de notes en Markdown.

## Fiabilité et code

- **Tests automatisés** : le projet n'a pour l'instant que des vérifications manuelles ponctuelles (des pilotes jetables décrits dans les `CLAUDE.md`) ; les transformer en suites versionnées sécuriserait les évolutions, avec `pytest` (`pytest-qt`) côté Python et QtTest/CTest côté C++.
- **Purge des brouillons masqués** : quand on ferme un fichier extérieur à `~/.noteeditor`, il quitte le panneau mais son brouillon, son entrée d'`index.json` et ses versions restent sur le disque (utile pour rouvrir la même note avec son historique) ; ils ne sont jamais nettoyés et s'accumulent. Une commande « Nettoyer » ou une purge des plus anciens réglerait cela.
- **Gestion des gros fichiers** : `QPlainTextEdit` supporte bien de grands documents, mais la coloration syntaxique par regex (`highlighters.py`) et le rendu complet de l'aperçu Markdown à chaque modification peuvent ralentir sur des fichiers volumineux — à vérifier/optimiser si l'usage s'y prête.
- **Icône d'application et intégration au menu des applications** : des lanceurs `noteeditor.desktop` existent (`Python/`, `Cpp/`) pour le démarrage automatique, mais sans icône, avec des chemins absolus propres à une machine, et sans installation dans le menu (`~/.local/share/applications`). Un script d'installation qui les génère avec les bons chemins, plus une icône, compléterait l'intégration Linux.
