# Claude Code Integration Test: Add Top Seller Badge

## Objective
Update the `docs/index.html` file to add a "TOP SELLER" badge to the Howling Wolves VST product card so it stands out on the homepage.

## Instructions for Claude Code
1. Open `docs/index.html` and locate the "Howling Wolves VST" product card in the "Featured products" grid.
2. Above the existing `<span class="card-badge">VST Plugin</span>`, add a new span element exactly like this:
   ```html
   <span class="card-badge" style="background-color: var(--accent-gold); color: #000; margin-right: 10px; font-weight: bold;">🔥 TOP SELLER</span>
   ```
3. Save the file.
4. Tell the user the task is complete so they can check the preview. You do NOT need to push to git for this test.
