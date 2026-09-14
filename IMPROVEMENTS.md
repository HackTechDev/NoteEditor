# Idées d'amélioration

Liste de pistes pour NoteEditor, classées par thème. Rien n'est engagé,
c'est juste un réservoir d'idées à picorer.

## Édition

- **Police et taille configurables** : actuellement fixées en dur (Monospace 11) dans `editor_widget.py`. Un réglage (menu ou raccourci `Ctrl+molette`) rendrait l'appli plus confortable selon l'écran.
- **Retour à la ligne automatique (word wrap)** activable/désactivable — utile pour du texte libre, gênant pour du code.
- **Indentation automatique** et **correspondance des parenthèses/accolades** pour les fichiers de code.
- **Plus de langages** pour la coloration syntaxique (`highlighters.py` ne couvre que Python/JSON/Markdown) : JS, HTML, CSS, YAML, Shell seraient des ajouts naturels.
- **Aperçu Markdown** en volet séparé pour les fichiers `.md`.
- **Correcteur orthographique** (via `pyspellchecker` ou l'intégration d'un dictionnaire système).

## Onglets et navigation

- **Menu contextuel sur les onglets** (clic droit) : fermer les autres, fermer tout, fermer à droite, dupliquer, renommer.
- **Renommer un onglet/brouillon** sans passer par « Enregistrer sous » — utile pour donner un nom clair aux notes sans titre.
- **Recherche/filtre dans le panneau Brouillons** (`drafts_browser.py`) quand la liste s'allonge avec l'usage.
- **Tri du panneau Brouillons** (par date, par nom) plutôt que le seul ordre actuel (date de modification).
- **Glisser-déposer** un fichier depuis l'explorateur pour l'ouvrir dans un nouvel onglet.

## Sauvegarde et données

- **Sauvegarde automatique continue** pendant la frappe (actuellement, le contenu n'est archivé qu'à la création, la fermeture d'un onglet ou celle de l'appli — une perte de courant en plein milieu d'édition ferait perdre le travail en cours).
- **Corbeille pour les brouillons supprimés** au lieu d'une suppression définitive immédiate (`session.delete_draft`), avec un délai de grâce ou une confirmation renforcée.
- **Historique des versions** d'un fichier (garder les N dernières sauvegardes), pour pouvoir revenir en arrière.
- **Détection de modification externe** : si le fichier ouvert a été modifié par un autre programme entre-temps, prévenir avant d'écraser.

## Interface

- **Thème sombre**, avec bascule manuelle ou suivi du thème système — les couleurs sont actuellement pensées uniquement pour un fond clair (`highlighters.py`, styles des onglets).
- **Barre de statut enrichie** : position ligne/colonne, nombre de mots/caractères, encodage.
- **Palette de commandes** (`Ctrl+Maj+P`) pour retrouver rapidement une action sans fouiller les menus.
- **Redimensionnement mémorisé** : retenir la taille de la fenêtre et la position du séparateur du panneau latéral d'une session à l'autre.

## Fiabilité et code

- **Tests automatisés** : le projet n'a pour l'instant que des vérifications manuelles ponctuelles ; une suite `pytest` (avec `pytest-qt`) sécuriserait les évolutions futures.
- **Gestion des gros fichiers** : `QPlainTextEdit` supporte bien de grands documents, mais la coloration syntaxique par regex (`highlighters.py`) peut ralentir sur des fichiers volumineux — à vérifier/optimiser si l'usage s'y prête.
- **Icône d'application** et fichier `.desktop` pour une intégration propre dans le menu des applications Linux.
