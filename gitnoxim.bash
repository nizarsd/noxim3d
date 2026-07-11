# 1. Stage only .cpp and .h files across all subdirectories
#git init

git add *.cpp *.h *.bash *.txt

# git remote set-url origin https://github.com/nizarsd/noxim3d.git

# 2. Commit the changes
git commit -m "Noxim oddevenBalanced fix and simulator performance improvements, sanity checked"

# 3. Push to GitHub
git push origin main