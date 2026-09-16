# Idées d'amélioration

Liste de pistes pour NoteEditor, classées par thème. Rien n'est engagé,
c'est juste un réservoir d'idées à picorer.

## Édition

- **Police et taille configurables** : actuellement fixées en dur (Monospace 11) dans `editor_widget.py`. Un réglage (menu ou raccourci `Ctrl+molette`) rendrait l'appli plus confortable selon l'écran.
- ~~**Retour à la ligne automatique (word wrap)** activable/désactivable~~ — fait ✅ (Python et C++) : bouton dans une nouvelle barre d'outils, appliqué globalement à tous les onglets.
- **Indentation automatique** et **correspondance des parenthèses/accolades** pour les fichiers de code.
- **Plus de langages** pour la coloration syntaxique (`highlighters.py` ne couvre que Python/JSON/Markdown) : JS, HTML, CSS, YAML, Shell seraient des ajouts naturels.
- **Aperçu Markdown** en volet séparé pour les fichiers `.md`.
- **Correcteur orthographique** (via `pyspellchecker` ou l'intégration d'un dictionnaire système).

## Onglets et navigation — fait ✅

Implémenté en Python et en C++ :

- Menu contextuel sur les onglets (clic droit) : fermer / fermer les autres / fermer à droite / fermer tout, dupliquer, renommer.
- Renommer un onglet/brouillon sans passer par « Enregistrer sous » (onglets sans fichier associé uniquement).
- Recherche et tri (date/nom) dans le panneau Brouillons.
- Glisser-déposer un fichier depuis l'explorateur pour l'ouvrir dans un nouvel onglet.

## Sauvegarde et données — fait ✅

Implémenté en Python et en C++ :

- Sauvegarde automatique continue (1,5s après la dernière frappe) dans `~/.noteeditor/drafts`, plus seulement à la création/fermeture d'un onglet ou de l'appli.
- Corbeille (`~/.noteeditor/trash`) : la suppression d'un brouillon est réversible, avec une boîte de dialogue pour restaurer ou supprimer définitivement.
- Historique des versions (`~/.noteeditor/versions`, 10 dernières) : chaque enregistrement archive le contenu précédent du fichier, consultable et restaurable depuis le menu contextuel d'un onglet.
- Détection de modification externe : si le fichier ouvert change sur le disque, l'appli propose de recharger au changement d'onglet ou au retour au premier plan.

## Interface

- **Thème sombre**, avec bascule manuelle ou suivi du thème système — les couleurs sont actuellement pensées uniquement pour un fond clair (`highlighters.py`, styles des onglets).
- **Barre de statut enrichie** : position ligne/colonne, nombre de mots/caractères, encodage.
- **Palette de commandes** (`Ctrl+Maj+P`) pour retrouver rapidement une action sans fouiller les menus.
- **Redimensionnement mémorisé** : retenir la taille de la fenêtre et la position du séparateur du panneau latéral d'une session à l'autre.

## Fiabilité et code

- **Tests automatisés** : le projet n'a pour l'instant que des vérifications manuelles ponctuelles ; une suite `pytest` (avec `pytest-qt`) sécuriserait les évolutions futures.
- **Gestion des gros fichiers** : `QPlainTextEdit` supporte bien de grands documents, mais la coloration syntaxique par regex (`highlighters.py`) peut ralentir sur des fichiers volumineux — à vérifier/optimiser si l'usage s'y prête.
- **Icône d'application** et fichier `.desktop` pour une intégration propre dans le menu des applications Linux.
