## Before You Get Started

### 1. Check the expiry of your SSH key
You can find it easily by searching for SSH in the gitlab search tab or under settings. If it is expiring soon it would be a good idea to set up a new one. This can be done by typing `ssh-keygen -t ed25519 -C "youremail@uwo.ca"` into your terminal. After that obtain the contents of the file by typing `cat ~/.ssh/id_ed25519.pub`. Next, press add new key in your gitlab and copy the output from the previous command. 

### 2. Clone the GIT repo using SSH
To clone the repository onto your device type `git clone ssh://git@gitlab.sci.uwo.ca:7999/courses/2026/01/COMPSCI3307/group45.git` into your terminal. If you would like this in a specific folder, navigate there before running the command.

## Whenever You Are Working on the Project
Follow the git workflow, below is a quick refresher:

### 1. Check Your status
`git status` - this tells you which branch you are currently working on and whether there are any uncommitted changes floating around

### 2. Get the Latest Version
`git pull` - make sure that you have the latest version of the code which prevents you from overwriting other people's work.

### 3. Stage Your Changes
`git add .` stages all of your current work, however it does not add it to your remote repository. To stage a specific file replace the . with its name, and to unstage a file use `git restore --staged .`.

### 4. Commit Your Changes
Once your work is staged and you are happy with it being part of your remote repository use `git commit -m "Message describing the changes you made"`. Keep in mind that this does not make your changes available to other people, just adds them to your local repository.

### 5. Push Your Changes
To make your work visible to others push your changes to the master (remote) branch with `git push`. 
