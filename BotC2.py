import json
import tkinter as tk
from tkinter import ttk, simpledialog, messagebox
from tkinter.filedialog import askopenfilename
from dataclasses import dataclass
import random
from pathlib import Path
import itertools
from dataclasses import dataclass, field
from typing import List


#### This is for BotC

colors = {"townsfolk": "Blue",
          "demon": "Orange",
          "minion": "Red",
          "outsider": "Green",
          "evil townsfolk":"Red"}

all_teams = ["townsfolk", "demon", "minion", "outsider", "evil townsfolk"]
# Role distribution rules
role_config = {
            5: {"townsfolk": 3, "outsider": 0, "minion": 1, "demon": 1},
            6: {"townsfolk": 3, "outsider": 1, "minion": 1, "demon": 1},
            7: {"townsfolk": 5, "outsider": 0, "minion": 1, "demon": 1},
            8: {"townsfolk": 5, "outsider": 1, "minion": 1, "demon": 1},
            9: {"townsfolk": 5, "outsider": 2, "minion": 1, "demon": 1},
            10: {"townsfolk": 7, "outsider": 0, "minion": 2, "demon": 1},
            11: {"townsfolk": 7, "outsider": 1, "minion": 2, "demon": 1},
            12: {"townsfolk": 7, "outsider": 2, "minion": 2, "demon": 1},
            13: {"townsfolk": 9, "outsider": 0, "minion": 3, "demon": 1},
            14: {"townsfolk": 9, "outsider": 1, "minion": 3, "demon": 1},
            15: {"townsfolk": 9, "outsider": 2, "minion": 3, "demon": 1},
    }
        
@dataclass
class Character:
    id: str
    name: str
    team: str
    ability: str
    first_night_order: int
    other_night_order: int
    firstNightReminder: str = ""
    otherNightReminder: str = ""
    reminders: List[str] = field(default_factory=list)


    def __repr__(self):
        return f"{self.name} ({self.team}): {self.ability}"
    
@dataclass
class Player:
    name: str
    team: str
    character: Character
    alive: bool = True
    poisoned: bool = False
    drunk: bool = False
    effects = {}

    def status(self):
        flags = []
        if not self.alive: flags.append("Dead")
        if self.poisoned: flags.append("Poisoned")
        if self.drunk: flags.append("Drunk")
        for key in self.effects.keys():
            if self.effects[key] == True:
                flags.append(key)
            
        return " / ".join(flags) if flags else "Healthy"