# Deploying Aether to Railway

Railway is a great choice because it can automatically build your Dockerfile.

## Step 1: Push to GitHub
Make sure your project is on GitHub.
1.  Initialize Git: `git init`
2.  Add All Files: `git add .`
3.  Commit: `git commit -m "Ready for deployment"`
4.  Create a Repo on GitHub and push:
    ```bash
    git remote add origin https://github.com/<your-username>/<repo-name>.git
    git push -u origin main
    ```

## Step 2: Create Railway Project
1.  Go to [railway.app](https://railway.app) and sign in.
2.  Click **"New Project"** -> **"Deploy from GitHub repo"**.
3.  Select your **Aether** repository.
4.  Click **"Deploy Now"**.

## Step 3: Configuration (Important)
Railway will detect the `Dockerfile` and start building. However, you need to ensure a few things:

1.  **RAM Usage**: The search engine loads data into RAM. If your data is huge (4GB+), you might hit the Free Tier limit (500MB).
    -   *Mitigation*: For the demo, ensure `searchengine.cpp` uses `limit=120` (which we did) or strictly limits the number of loaded documents if memory is an issue.

2.  **Public URL**:
    -   Once deployed, go to the **Settings** tab of your service.
    -   Scroll to **"Networking"** -> **"Generate Domain"**.
    -   You will get a URL like `aether-production.up.railway.app`.

3.  **Frontend Config**:
    -   The `Dockerfile` builds the frontend *inside* the image and serves it at `/`.
    -   So, your React app will natively talk to the backend on the same domain. You **DO NOT** need to change `LOCAL_API` or `CLOUD_API` in `App.jsx` if you serve it this way, because relative paths (or same-origin) would work if we configured it. 
    -   *Current Setup*: `App.jsx` points to `https://aether-engine.up.railway.app`. If your generated domain is different, **you must update `CLOUD_API` in `App.jsx` and push again.**

## Troubleshooting
-   **Build Failure**: Check the "Build Logs". 
-   **"Killed"**: Usually means Out of Memory (OOM). Increase the plan or reduce dataset size.
