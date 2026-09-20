# Idées d'amélioration

Liste de pistes pour NoteEditor, classées par thème. Rien n'est engagé,
c'est juste un réservoir d'idées à picorer. Les fonctionnalités déjà
implémentées sont documentées dans `README.md` et `FEATURES.md`, pas ici.

## Édition

- **Police et taille configurables** : actuellement fixées en dur (Monospace 11) dans `editor_widget.py` et `Editor.cpp`. Un réglage (menu ou raccourci `Ctrl+molette`) rendrait l'appli plus confortable selon l'écran.
- **Indentation automatique** et **correspondance des parenthèses/accolades** pour les fichiers de code.
- **Plus de langages** pour la coloration syntaxique (`highlighters.py` / `Highlighters.cpp` ne couvrent que Python/JSON/Markdown) : JS, HTML, CSS, YAML, Shell seraient des ajouts naturels.
- **Aperçu Markdown** : synchroniser son défilement avec celui de l'éditeur, et pouvoir exporter le rendu en HTML.
- **Correcteur orthographique** (via `pyspellchecker` ou l'intégration d'un dictionnaire système).

## Interface

- **Thème sombre**, avec bascule manuelle ou suivi du thème système — les couleurs sont actuellement pensées uniquement pour un fond clair (`highlighters.py`, styles des onglets, aperçu Markdown).
- **Palette de commandes** (`Ctrl+Maj+P`) pour retrouver rapidement une action sans fouiller les menus.

## Session et fichiers

- **Restaurer aussi la position du curseur et le défilement de chaque onglet** : la restauration de session rend le texte, le fichier associé, l'état modifié, l'onglet actif et la fenêtre, mais pas encore l'endroit exact où l'on travaillait dans chaque note.
- **Mémoriser les réglages d'affichage** entre deux lancements : le retour automatique à la ligne est réactivé et l'aperçu Markdown masqué à chaque démarrage.
- **« Enregistrer sous » : filtres Markdown** — la fenêtre « Ouvrir » propose `.txt` et `.md`, mais « Enregistrer sous » n'affiche encore que `.txt`.
- **Éviter les doublons du panneau Brouillons lors d'un « Enregistrer sous »** vers un chemin qui a déjà une note : seule l'ouverture d'un fichier réutilise aujourd'hui l'entrée existante.

## Fiabilité et code

- **Tests automatisés** : le projet n'a pour l'instant que des vérifications manuelles ponctuelles (des pilotes jetables décrits dans les `CLAUDE.md`) ; les transformer en suites versionnées sécuriserait les évolutions, avec `pytest` (`pytest-qt`) côté Python et QtTest/CTest côté C++.
- **Gestion des gros fichiers** : `QPlainTextEdit` supporte bien de grands documents, mais la coloration syntaxique par regex (`highlighters.py`) et le rendu complet de l'aperçu Markdown à chaque modification peuvent ralentir sur des fichiers volumineux — à vérifier/optimiser si l'usage s'y prête.
- **Icône d'application et intégration au menu des applications** : des lanceurs `noteeditor.desktop` existent (`Python/`, `Cpp/`) pour le démarrage automatique, mais sans icône, avec des chemins absolus propres à une machine, et sans installation dans le menu (`~/.local/share/applications`). Un script d'installation qui les génère avec les bons chemins, plus une icône, compléterait l'intégration Linux.
