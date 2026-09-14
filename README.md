# NoteEditor

Éditeur de texte à onglets, écrit en Python avec PyQt6.

## Fonctionnalités

- Onglets multiples (fermables, réordonnables), onglet actif bien visible
- Numéros de ligne avec surlignage de la ligne courante
- Coloration syntaxique automatique selon l'extension : Python (`.py`), JSON (`.json`), Markdown (`.md`)
- Recherche / remplacement (`Ctrl+F` / `Ctrl+H`) : suivant, précédent, remplacer, tout remplacer
- Nouveaux onglets nommés par date/heure (`aammjj_hhmmssmm`)
- Session persistante : à la fermeture, tous les onglets (contenu, fichier associé, état modifié, onglet actif) sont sauvegardés automatiquement dans `~/.noteeditor` et restaurés tels quels au prochain lancement

## Installation

```bash
pip install -r requirements.txt
```

## Lancement

```bash
python3 main.py
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

| Fichier             | Rôle                                                             |
|----------------------|-------------------------------------------------------------------|
| `main.py`            | Fenêtre principale, gestion des onglets, menus, ouverture/enregistrement |
| `editor_widget.py`   | Widget d'édition (gouttière de numéros de ligne, ligne courante)   |
| `highlighters.py`    | Coloration syntaxique (Python, JSON, Markdown)                    |
| `find_replace.py`    | Boîte de dialogue de recherche / remplacement                     |
| `session.py`         | Sauvegarde et restauration de la session dans `~/.noteeditor`     |

## Session (`~/.noteeditor`)

- `~/.noteeditor/session.json` : liste des onglets ouverts (fichier associé, nom par défaut, état modifié, onglet actif)
- `~/.noteeditor/drafts/` : contenu de chaque onglet au moment de la fermeture

Cette copie de secours n'écrase jamais le fichier d'origine sur le disque : seul un `Enregistrer` explicite (`Ctrl+S`) modifie le fichier réel.
