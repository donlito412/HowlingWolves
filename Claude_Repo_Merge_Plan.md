# Repository Consolidation Plan for Claude

The target is to safely merge the entire `Wolf Instruments` development workspace (VST code + the updated `docs` website) into the live website repository (`hw-temp/HowlingWolves`), allowing the user to review the final folder structure before anything is deleted or pushed live.

## Execution Steps for Claude

Claude needs to execute the following commands in the user's Mac terminal to perform a safe, non-destructive merge.

### Step 1: Safely Copy Files
Run the following `rsync` command. This will copy all files from the main `Wolf Instruments` folder into the `hw-temp/HowlingWolves` folder. 
*   It explicitly ignores the `.git` folder (so it doesn't break the repository connection).
*   It explicitly ignores the `hw-temp` folder (to avoid infinite copying loops).
*   **It does not delete** any files that might currently be in `HowlingWolves` but aren't in the main folder.

```bash
cd "/Volumes/easystore/Projects/Music Production/Wolf Instruments"
rsync -av --exclude='.git' --exclude='hw-temp' ./ hw-temp/HowlingWolves/
```

### Step 2: Pause for User Review
Once the transfer completes, Claude must **STOP and wait** for the user. 
The user will open `hw-temp/HowlingWolves/` in Finder or their editor to visually verify that all the VST source files (`Source/`, `CMakeLists.txt`, etc.) and the updated `docs/` website are all sitting together correctly.

### Step 3: Commit and Push (Upon Approval)
Once the user confirms the folder structure looks perfect, Claude will run the following commands to commit the massive consolidation to GitHub, which will automatically deploy the updated site to the live `wolfpaccaudio.com` domain.

```bash
cd "/Volumes/easystore/Projects/Music Production/Wolf Instruments/hw-temp/HowlingWolves"
git add .
git commit -m "Consolidate VST workspace and updated website"
git push origin main
```
