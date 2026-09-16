# Idées d'amélioration

Liste de pistes pour NoteEditor, classées par thème. Rien n'est engagé,
c'est juste un réservoir d'idées à picorer. Les fonctionnalités déjà
implémentées sont documentées dans `README.md`, pas ici.

## Édition

- **Police et taille configurables** : actuellement fixées en dur (Monospace 11) dans `editor_widget.py`. Un réglage (menu ou raccourci `Ctrl+molette`) rendrait l'appli plus confortable selon l'écran.
- **Indentation automatique** et **correspondance des parenthèses/accolades** pour les fichiers de code.
- **Plus de langages** pour la coloration syntaxique (`highlighters.py` ne couvre que Python/JSON/Markdown) : JS, HTML, CSS, YAML, Shell seraient des ajouts naturels.
- **Aperçu Markdown** en volet séparé pour les fichiers `.md`.
- **Correcteur orthographique** (via `pyspellchecker` ou l'intégration d'un dictionnaire système).

## Interface

- **Thème sombre**, avec bascule manuelle ou suivi du thème système — les couleurs sont actuellement pensées uniquement pour un fond clair (`highlighters.py`, styles des onglets).
- **Palette de commandes** (`Ctrl+Maj+P`) pour retrouver rapidement une action sans fouiller les menus.

## Fiabilité et code

- **Tests automatisés** : le projet n'a pour l'instant que des vérifications manuelles ponctuelles ; une suite `pytest` (avec `pytest-qt`) sécuriserait les évolutions futures.
- **Gestion des gros fichiers** : `QPlainTextEdit` supporte bien de grands documents, mais la coloration syntaxique par regex (`highlighters.py`) peut ralentir sur des fichiers volumineux — à vérifier/optimiser si l'usage s'y prête.
- **Icône d'application** et fichier `.desktop` pour une intégration propre dans le menu des applications Linux.
