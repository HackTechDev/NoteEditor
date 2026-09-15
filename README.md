# NoteEditor

Éditeur de texte à onglets, écrit en Python avec PyQt6.

## Fonctionnalités

- Onglets multiples (fermables, réordonnables), onglet actif bien visible
- Bouton **+** pour créer un nouvel onglet, collé juste après le dernier onglet (style Gedit) ; se déplace automatiquement à côté des flèches de défilement quand les onglets débordent de la largeur disponible
- Numéros de ligne avec surlignage de la ligne courante
- Coloration syntaxique automatique selon l'extension : Python (`.py`), JSON (`.json`), Markdown (`.md`)
- Recherche / remplacement (`Ctrl+F` / `Ctrl+H`) : suivant, précédent, remplacer, tout remplacer
- Nouveaux onglets nommés par date/heure (`aammjj_hhmmssmm`)
- `Ctrl+S` sur un onglet sans fichier associé l'enregistre directement dans `~/.noteeditor/docs/` (sous son nom par défaut), sans ouvrir de boîte de dialogue ; `Ctrl+Shift+S` (Enregistrer sous) permet de choisir un autre emplacement
- Session persistante : à la fermeture, tous les onglets (contenu, fichier associé, état modifié, onglet actif) sont sauvegardés automatiquement dans `~/.noteeditor` et restaurés tels quels au prochain lancement
- Chaque onglet est archivé dans `~/.noteeditor` dès sa création, et à nouveau à chaque fermeture (croix ou Ctrl+W, sans demander de confirmation même en cas de modifications non enregistrées)
- Panneau « Brouillons » à gauche : liste tous les onglets dont le contenu a été archivé dans `~/.noteeditor` (ceux actuellement ouverts sont marqués « (ouvert) »), avec l'entrée de l'onglet actif surlignée ; clic pour rouvrir ou basculer dessus, clic droit pour supprimer définitivement
- Menu Aide → À propos

## Installation

```bash
cd Python
pip install -r requirements.txt
```

## Lancement

```bash
cd Python
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

Le code Python vit dans `Python/` (d'autres implémentations pourraient un jour rejoindre le dépôt dans leur propre répertoire).

| Fichier                      | Rôle                                                             |
|--------------------------------|-------------------------------------------------------------------|
| `Python/main.py`            | Fenêtre principale, gestion des onglets, menus, ouverture/enregistrement |
| `Python/editor_widget.py`   | Widget d'édition (gouttière de numéros de ligne, ligne courante)   |
| `Python/highlighters.py`    | Coloration syntaxique (Python, JSON, Markdown)                    |
| `Python/find_replace.py`    | Boîte de dialogue de recherche / remplacement                     |
| `Python/session.py`         | Sauvegarde et restauration de la session dans `~/.noteeditor`     |
| `Python/drafts_browser.py`  | Panneau latéral listant les brouillons (ouverts et fermés)         |

## Session (`~/.noteeditor`)

- `~/.noteeditor/session.json` : liste des onglets actuellement ouverts (fichier associé, nom par défaut, état modifié, onglet actif)
- `~/.noteeditor/index.json` : métadonnées de tous les brouillons jamais sauvegardés (pour l'affichage dans le panneau « Brouillons »)
- `~/.noteeditor/drafts/` : contenu de chaque onglet, conservé même après la fermeture de son onglet
- `~/.noteeditor/docs/` : fichiers réels créés par `Ctrl+S` depuis un onglet sans titre

Cette copie de secours n'écrase jamais le fichier d'origine sur le disque : seul un `Enregistrer` explicite (`Ctrl+S`) modifie le fichier réel (que ce soit dans `~/.noteeditor/docs/` pour un onglet sans titre, ou à l'emplacement d'origine pour un fichier ouvert ailleurs). Les brouillons ne sont supprimés que manuellement, depuis le panneau latéral.
