import json
import tkinter as tk
from tkinter import ttk, simpledialog, messagebox
from tkinter.filedialog import askopenfilename
from dataclasses import dataclass
import random
from pathlib import Path

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

    def status(self):
        flags = []
        if not self.alive: flags.append("Dead")
        if self.poisoned: flags.append("Poisoned")
        if self.drunk: flags.append("Drunk")
        return ", ".join(flags) if flags else "Healthy"

class StorytellerApp:
    def __init__(self, master):
        text_size = 20
        self.first_night = True  # Add this in __init_
        self.master = master
        
        self.master_frame = tk.Frame(master)
        self.master_frame.grid(row=0, column=0, sticky="nsew")
        master.grid_columnconfigure(0, weight=1)
        #self.master.grid_rowconfigure(0, weight=1)
        #self.master.grid_columnconfigure(0, weight=1)
        
        self.frame = tk.Frame(self.master_frame)
        self.frame.grid(row=0, column=0)
        self.master_frame.grid_columnconfigure(0, weight=1) 
        
        self.night_controls = tk.Frame(self.master_frame)
        #self.night_controls.grid(row=1, column=0, columnspan=2, sticky="nsew")
        self.night_controls.grid_remove()  # Hide by default
        
        self.day_controls = tk.Frame(self.master_frame)
        self.day_controls.grid(row=1,column=0)
        self.day_controls.grid_columnconfigure((0, 1, 2), weight=1)
        for i in range(3):  # Assuming 3 columns of buttons
            self.day_controls.grid_columnconfigure(i, weight=1)
        
        self.bluff_frame = tk.Frame(self.master_frame)
        self.bluff_frame.grid(pady=5)
        
        #getting screen width and height of display
        width= master.winfo_screenwidth() 
        height= master.winfo_screenheight()
        #setting tkinter window size
        master.geometry("%dx%d" % (width, height))
        master.title("Blood on the Clocktower - Storyteller Tool")

        self.character_db = {}   # All characters by ID
        self.script_characters = []  # List of Character objects in current script
        self.players: list[Player] = []
        self.bluffs: list[Character] = []
        self.day = 1

        
        self.load_character_db()
        self.load_script()

        self.header = tk.Label(master, text=f"Day {self.day}", font=("Helvetica", 16))
        self.header.grid()



        button_width = 20  # number of characters wide
        button_height = 2  # optional: number of text lines high

        self.add_player_button = tk.Button(self.day_controls, 
                                            text="Add Player",
                                            command=self.add_player_dialog,
                                            font=("Helvetica",text_size),
                                            width=button_width,
                                            height=button_height
                                            )
        self.add_player_button.grid(row=0, column=0, padx=10, pady=10, sticky="ew")

        self.night_button = tk.Button(self.day_controls,
                                      text="Start Night Phase",
                                      command=self.start_night,
                                      font=("Helvetica", text_size),
                                      width=button_width,
                                      height=button_height
                                      )
        self.night_button.grid(row=0, column=1, padx=10, pady=10)

        self.next_day_button = tk.Button(self.day_controls,
                                         text="Next Day",
                                         command=self.advance_day,
                                         font=("Helvetica", text_size),
                                         width=button_width,
                                         height=button_height
                                         )
        self.next_day_button.grid(row=0, column=2, padx=10, pady=10)

        self.random_button = tk.Button(self.day_controls,
                                       text="Distribute Characters",
                                       command=self.assign_random_characters, 
                                       font=("Helvetica", text_size),
                                       width=button_width,
                                       height=button_height
                                       )
        self.random_button.grid(row=1, column=0, padx=10, pady=10)

        self.generate_button = tk.Button(self.day_controls,
                                         text="Generate Game",
                                         command=self.generate_game, 
                                         font=("Helvetica", text_size),
                                         width=button_width,
                                         height=button_height
                                         )
        self.generate_button.grid(row=1, column=1, padx=10, pady=10)
        
        self.message_button = tk.Button(self.day_controls,
                                        text="Message",
                                        command=self.send_message_dialog,
                                        font=("Helvetica", text_size),
                                        width=button_width,
                                        height=button_height
                                        )
        self.message_button.grid(row=1, column=2, padx=10, pady=10)
        
        self.show_all_var = tk.BooleanVar()
        tk.Checkbutton(self.master, text="Show all characters in night", variable=self.show_all_var).grid()
        
        
        self.update_display()

    def load_character_db(self):
            #path_m = "/home/lukas/Documents/BotC/Master_BotC.json"
            path_m = Path(__file__).parent / "Master_BotC.json"
            with open(path_m, "r") as f:
                data = json.load(f)

            for info in data:
                cid = info["id"]
                char = Character(
                    id=cid,
                    name=info["name"],
                    team=info["team"],
                    ability=info["ability"],
                    first_night_order=info.get("first_night_order", 999),
                    other_night_order=info.get("other_night_order", 999),
                    firstNightReminder=info.get("firstNightReminder", ""),
                    otherNightReminder=info.get("otherNightReminder", "")
                )
                self.character_db[cid] = char


    def load_script(self):
        initial_dir = Path(__file__).parent / "scripts"
        path = askopenfilename(initialdir=initial_dir, title="Enter script.json:", filetypes=[("JSON", ".json")])
        #path = askopenfilename(initialdir="/home/lukas/Documents/BotC",title= "Enter script.json:", filetypes=[("JSON", ".json")])
        with open(path, "r") as f:
            ids = json.load(f)
        self.script_characters = [self.character_db[c["id"]] for c in ids if c["id"] in self.character_db]

    def add_player_dialog(self, editing_player=None):
        text_size = 20
        dialog = tk.Toplevel(self.master)
        dialog.title("Add Player" if not editing_player else "Edit Player")

        tk.Label(dialog, text="Player name:", font=("Helvetica", text_size)).grid(row=0, column=0)
        name_entry = tk.Entry(dialog, font=("Helvetica", text_size))
        name_entry.grid(row=0, column=1)

        if editing_player:
            name_entry.insert(0, editing_player.name)

        # Dropdown setup
        tk.Label(dialog, text="Assign character:", font=("Helvetica", text_size)).grid(row=1, column=0)

        # Filter available characters
        if editing_player:
            available_chars = self.script_characters
        else:
            assigned_ids = [p.character.id for p in self.players if p != editing_player]
            available_chars = [c for c in self.script_characters if c.id not in assigned_ids]

        char_var = tk.StringVar()
        if editing_player:
            char_var.set(editing_player.character.name)
        else:
            char_var.set(available_chars[0].name if available_chars else "")

        # Character selection
        menu = tk.OptionMenu(dialog, char_var, *[c.name for c in available_chars])
        menu.config(font=("Helvetica", text_size))
        content = menu.nametowidget(menu.menuname)
        content.config(font=("Helvetica", text_size))
        menu.grid(row=1, column=1)
        
        # Colorize menu items
        menu["menu"].delete(0, "end")
        for c in available_chars:
            color = colors[c.team]
            menu["menu"].add_command(
                label=c.name,
                command=lambda name=c.name: char_var.set(name),
                background=color
            )
            
        # team selection
        if editing_player:
            tk.Label(dialog, text="Assign team:", font=("Helvetica", text_size)).grid(row=2, column=0)
            new_team = tk.StringVar()
            new_team.set(editing_player.team)
           
            team_menu = tk.OptionMenu(dialog, new_team, *[a for a in all_teams])
            team_menu.config(font=("Helvetica", text_size))
            team_content = team_menu.nametowidget(team_menu.menuname)
            team_content.config(font=("Helvetica", text_size))
            team_menu.grid(row=2, column=1)
         # Colorize menu items
            team_menu["menu"].delete(0, "end")
            for t in all_teams:
                color = colors[t]
                team_menu["menu"].add_command(
                    label=t,
                    command=lambda tea=t: new_team.set(tea),
                    background=color
                )

        def confirm():
            name = name_entry.get()
            selected_name = char_var.get()

            char_obj = next((c for c in self.script_characters if c.name == selected_name), None)
            if not name or not char_obj:
                return

            if editing_player:
                selected_team = new_team.get()
                editing_player.name = name
                editing_player.character = char_obj
                editing_player.team = selected_team
                print(selected_team)
            else:
                new_player = Player(name=name, character=char_obj, team = char_obj.team)
                self.players.append(new_player)

            dialog.destroy()
            self.update_display()

        tk.Button(dialog, text="OK", command=confirm, font=("Helvetica", text_size)).grid(row=3, column=1)

    
    def update_display(self):
        text_size = 20
        for widget in self.frame.winfo_children():
            widget.destroy()

        for i, player in enumerate(self.players):
            row = tk.Frame(self.frame)
            row.grid(row=i, sticky="w")
            tk.Button(row, text="Edit", command=lambda p=player: self.add_player_dialog(editing_player=p), font=("Helvetica", text_size)).grid(row=0, column=0)

            tk.Label(row, text=f"{player.name} — {player.character.name}", width=30,bg=colors[player.team], font=("Helvetica", text_size)).grid(row=0, column=2)
            tk.Label(row, text=player.status(), width=25, font=("Helvetica", text_size)).grid(row=0, column=3)
            tk.Button(row, text="Kill", command=lambda p=player: self.toggle_status(p, "alive"), font=("Helvetica", text_size)).grid(row=0, column=4)
            tk.Button(row, text="Poison", command=lambda p=player: self.toggle_status(p, "poisoned"), font=("Helvetica", text_size)).grid(row=0, column=5)
            tk.Button(row, text="Drunk", command=lambda p=player: self.toggle_status(p, "drunk"), font=("Helvetica", text_size)).grid(row=0, column=6)

        
    def show_bluffs_window(self):
        if not self.bluffs:
            messagebox.showinfo("Bluffs", "No bluffs selected.")
            return

        window = tk.Toplevel(self.master)
        #getting screen width and height of display
        width= window.winfo_screenwidth() 
        height= window.winfo_screenheight()
        #setting tkinter window size
        window.geometry("%dx%d" % (width, height))
        window.title("Bluff Characters")

        frame = tk.Frame(window, pady=5)
        frame.grid(padx=10)
            
        tk.Label(frame, text="Demon Bluffs", font=("Helvetica", 40), anchor=tk.CENTER).grid()
        for char in self.bluffs:


            tk.Label(frame, text=f"{char.name} ({char.team})", font=("Helvetica", 35), anchor = tk.CENTER, fg=colors[char.team]).grid(anchor="w")
            tk.Label(frame, text=char.ability, wraplength=500, fg="gray",font=("Helvetica", 30), anchor = tk.CENTER).grid(anchor="w")

            
    def choose_bluffs(self):
        # Characters not assigned to players
        assigned_ids = {p.character.id for p in self.players}
        available = [c for c in self.script_characters if c.id not in assigned_ids and c.team in ("townsfolk", "outsider")]
        
        self.bluffs = random.sample(available, k=min(3, len(available)))
        
        # Clear previous bluff widgets
        for widget in self.bluff_frame.winfo_children():
            widget.destroy()

        # Create and display bluff info
        if self.bluffs:
            tk.Label(self.bluff_frame, text="Bluffs: " + ", ".join([b.name for b in self.bluffs]), font=("Helvetica", 12, "bold")).grid(pady=5)
            tk.Button(self.bluff_frame, text="View Bluff Details", command=self.show_bluffs_window).grid()
        self.update_display()        

    def toggle_status(self, player: Player, status_type: str):
        if status_type == "alive":
            player.alive = not player.alive
        elif status_type == "poisoned":
            player.poisoned = not player.poisoned
        elif status_type == "drunk":
            player.drunk = not player.drunk
        self.update_display()

    def advance_day(self):
        self.day += 1
        self.first_night = False  # Only true on day 1
        self.header.config(text=f"Day {self.day}")
        self.update_display()

    def start_night(self):
        
        self.day_controls.grid_remove()  # Hide day controls
        self.night_controls.grid() # show night controls
    
        if self.show_all_var.get():
            night_players = self.players
        else:
            # filtered list as before

            if self.first_night:
                night_players = [p for p in self.players if p.character.firstNightReminder]
            else:
                night_players = [p for p in self.players if p.character.otherNightReminder]

        # Sort by night order
        if self.first_night:
            night_ordered = sorted(night_players, key=lambda p: p.character.first_night_order)
        else:
            night_ordered = sorted(night_players, key=lambda p: p.character.other_night_order)
        
        
        self.night_window(night_ordered, 0)
        self.update_display()

    def night_window(self, ordered_players, index):
        for widget in self.night_controls.winfo_children():
            widget.destroy()
        
        text_size = 16
        
        
        
        if index >= len(ordered_players):
            messagebox.showinfo("Night Phase", "Night complete.")
            self.end_night()
            return

        player = ordered_players[index]
        char = player.character
        reminder = char.firstNightReminder if self.first_night else player.character.otherNightReminder
        
        if not reminder:
            reminder = "(No night action)"
            
        tk.Label(self.night_controls, text="Night Phase", font=("Helvetica", 18, "bold")).grid(row=0, column=0, columnspan=2, pady=10)
        tk.Label(self.night_controls, text=f"{char.name} ({char.team}) ({player.status()})", fg=colors[player.team], font=("Helvetica", text_size)).grid(row=1,column=0, sticky="w")
        tk.Label(self.night_controls, text=f"Player: {player.name}", fg=colors[player.team], font=("Helvetica", text_size)).grid(row=2,column=0, sticky="w")
        tk.Label(self.night_controls, text=reminder, wraplength=400, font=("Helvetica", text_size)).grid(row=3,column=0, sticky="w")
        tk.Label(self.night_controls, text=f"Ability: {char.ability}", wraplength=400, fg="gray", font=("Helvetica", text_size)).grid(row=2,column=2, sticky="w")

        def next_step():
            for widget in self.night_controls.winfo_children():
                widget.destroy()
            self.night_window(ordered_players, index + 1)
            
        tk.Button(self.night_controls, text="Next", command=next_step, font=("Helvetica", text_size)).grid(row=1,column=4)
        tk.Button(self.night_controls, text="End Night", font=("Helvetica", 14), command=self.end_night).grid(row=2,column=4)

        
    def end_night(self):
        self.first_night = False
        self.night_controls.grid_remove()
        self.day_controls.grid()
        self.update_display()

    def assign_random_characters(self):
        num_players = len(self.players)
        if num_players == 0:
            messagebox.showwarning("No Players", "Add players before assigning characters.")
            return

        config = role_config.get(num_players)
        if not config:
            messagebox.showerror("Unsupported Player Count", "Only 5–15 players are supported.")
            return

        # Group script characters by team
        team_pools = {
            "townsfolk": [c for c in self.script_characters if c.team == "townsfolk"],
            "outsider": [c for c in self.script_characters if c.team == "outsider"],
            "minion": [c for c in self.script_characters if c.team == "minion"],
            "demon": [c for c in self.script_characters if c.team == "demon"]
        }

        selected_characters = []

        try:
            for team, count in config.items():
                if len(team_pools[team]) < count:
                    raise ValueError(f"Not enough characters in script for team: {team}")
                selected_characters += random.sample(team_pools[team], count)
        except ValueError as e:
            messagebox.showerror("Character Assignment Error", str(e))
            return

        # Shuffle characters and assign to players
        random.shuffle(selected_characters)
        for player, char in zip(self.players, selected_characters):
            player.character = char
            player.team = char.team
            player.alive = True
            player.poisoned = False
            player.drunk = False
            
        self.choose_bluffs()
        self.first_night = True
        self.day = 1
        self.header.config(text=f"Day {self.day}")
        self.update_display()
    
    def generate_game(self):
        num_players = simpledialog.askinteger("Number of Players", "Enter number of players (5–15):", minvalue=5, maxvalue=15)
        if not num_players:
            return

        # Clear existing players
        self.players = []


        config = role_config.get(num_players)
        if not config:
            messagebox.showerror("Invalid", "Unsupported number of players.")
            return

        # Group script characters by team
        from random import sample, shuffle

        team_pool = {
            "townsfolk": [c for c in self.script_characters if c.team == "townsfolk"],
            "outsider": [c for c in self.script_characters if c.team == "outsider"],
            "minion": [c for c in self.script_characters if c.team == "minion"],
            "demon": [c for c in self.script_characters if c.team == "demon"]
        }

        # Ensure we have enough unique characters
        for team, needed in config.items():
            if len(team_pool[team]) < needed:
                messagebox.showerror("Not enough characters",
                                    f"Script does not have enough unique {team} characters (needed: {needed}).")
                return

        # Randomly select characters
        selected = []
        for team, count in config.items():
            selected.extend(sample(team_pool[team], count))  # sample = no duplicates

        shuffle(selected)

        # Create players and assign
        for i in range(num_players):
            char = selected[i]
            new_player = Player(name=f"Player {i+1}", team=char.team, character=char)
            self.players.append(new_player)

        self.choose_bluffs()
        self.first_night = True
        self.day = 1
        self.header.config(text=f"Day {self.day}")
        self.update_display()

    def send_message_dialog(self):
        text_size = 20
        dialog = tk.Toplevel(self.master)
        dialog.title(f"Send Message")
        #dialog.geometry("400x300")
        
        frame = tk.Frame(dialog)
        frame.pack(padx=10, pady=10)

        tk.Label(frame, text=f"Send a message", font=("Helvetica", 14)).grid(row=0, column=0, columnspan=2, pady=10)

        # Message type dropdown
        tk.Label(frame, text="Message type:", font=("Helvetica", text_size)).grid(row=1, column=0, sticky="e")
        msg_var = tk.StringVar()
        msg_var.set("You are now")
        options = ["You are now", "You were chosen by", "You were seen as", "You learn that", "You feel like"]
        msg_menu = tk.OptionMenu(frame, msg_var, *options)
        msg_menu.config(font=("Helvetica", text_size))
        msg_menu.grid(row=1, column=1, sticky="w")

        # Character dropdown
        tk.Label(frame, text="Character:", font=("Helvetica", text_size)).grid(row=2, column=0, sticky="e")
        char_var = tk.StringVar()
        char_names = [char.name for char in self.script_characters]
        if char_names:
            char_var.set(char_names[0])

        char_menu = tk.OptionMenu(frame, char_var, *char_names)
        char_menu.config(font=("Helvetica", text_size))
        char_menu.grid(row=2, column=1, sticky="w")

        # Colorize character dropdown
        menu_widget = char_menu["menu"]
        menu_widget.delete(0, "end")
        for char in self.script_characters:
            color = colors.get(char.team, "white")
            menu_widget.add_command(
                label=char.name,
                command=lambda name=char.name: char_var.set(name),
                background=color
            )
            
        # Output label
        result_label = tk.Label(frame, text="", wraplength=350, font=("Helvetica", 12), fg="blue")
        result_label.grid(row=3, column=0, columnspan=2, pady=10)

        def send():
            msg = f"{msg_var.get()} {char_var.get()}"
            dialog.destroy()  # Close the original selection dialog

            # Create new large message display window
            msg_window = tk.Toplevel(self.master)
            msg_window.title(f"Message")
            msg_window.geometry("1800x900")  # You can adjust size as needed

            tk.Label(msg_window, text=msg_var.get(), wraplength=480, font=("Helvetica", 35), fg="black").pack(expand=True, fill="both", padx=20, pady=10)
            tk.Label(msg_window, text=char_var.get(), wraplength=480, font=("Helvetica", 35), fg="black").pack(expand=True, fill="both", padx=20, pady=10)
            tk.Button(msg_window, text="Close", command=msg_window.destroy, font=("Helvetica", 14)).pack(pady=10)


        # Buttons
        tk.Button(frame, text="Send", command=send, font=("Helvetica", 12)).grid(row=4, column=0, pady=10)
        tk.Button(frame, text="Close", command=dialog.destroy, font=("Helvetica", 12)).grid(row=4, column=1, pady=10)

# Start app
if __name__ == "__main__":
    root = tk.Tk()
    app = StorytellerApp(root)
    root.mainloop()
