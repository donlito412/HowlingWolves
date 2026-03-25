# Wolf Pacc Lead Magnet Fix - Instructions for Claude

Claude, you need to fix the lead magnet delivery on the website and assemble the sample pack. The user uses **Gumroad** exclusively for their storefront and email capture. You incorrectly implemented Formspree in the website code. 

Please execute the following steps immediately to fix this and assemble the files:

## 1. Rip out the Formspree Code
- Open `docs/free-sounds/index.html`.
- Find the entire `<form>` block (around line 242) that uses `action="https://formspree.io/f/mgonzlqj"` and delete it completely.
- Replace that entire form section with a simple, high-converting call-to-action button that links directly to the user's free Gumroad product page, or embed the Gumroad checkout popup directly using their provided scripts.
- Ensure the user does not have to deal with any third-party email forms except Gumroad.

## 2. Create Staging Folders for the Free Pack
- In the root of the project (`/Volumes/Lito's Hard Drive/Music Production/Wolf Instruments/`), create a temporary folder called `Wolf_Pacc_Free_Teaser_Pack`.
- Inside that new folder, create two subfolders: `Melodies` and `Drums`.

## 3. Gather the Melodies
- Navigate to `docs/audio/Melody Samples/`.
- Copy all 5 `.wav` files from that folder and place them into the new `Wolf_Pacc_Free_Teaser_Pack/Melodies/` staging folder.

## 4. Gather the Drums
- Locate `Synthwave Drum Pack.zip` in the root of the project.
- Extract the zip file and place all 16 drum sounds directly into the `Wolf_Pacc_Free_Teaser_Pack/Drums/` staging folder.
- Clean up the extraction artifact so no messy temporary folders are left behind.

## 5. Create the ReadMe File
- Inside the main `Wolf_Pacc_Free_Teaser_Pack` folder, create a text file exactly named `Welcome_ReadMe.txt`.
- Add the following exact text to it:

```text
Thank you for downloading the Wolf Pacc Audio Free Teaser Pack!

Inside you will find 5 exclusive melodies and the complete Synthwave Drum Pack. This represents the exact quality of what we build at Wolf Pacc Audio.

Want to make your own melodies like these?
Get the Howling Wolves VST: https://wolfpaccaudio.com/vst

Looking for more premium drum kits, beats, and expansion packs?
Check out our full catalog: https://wolfpaccaudio.com

Stay locked in. We drop new sounds every single month.

- Wolf Pacc Audio
https://wolfpaccaudio.com
```

## 6. Zip Request
- Zip the entire `Wolf_Pacc_Free_Teaser_Pack` folder into an archive named `Wolf_Pacc_Free_Teaser_Pack.zip`.
- Since we are moving to Gumroad for the lead magnet delivery, the user will upload this `.zip` directly to Gumroad as the product file. You can leave the final `.zip` safely in the root `Wolf Instruments` folder.
- Delete the unzipped `Wolf_Pacc_Free_Teaser_Pack` staging folder from the root so the workspace stays perfectly clean.
