# 1. Stage only .cpp and .h files across all subdirectories
#git init

CM=${1:-updates}
git add *.cpp *.h *.bash *.txt

# git remote set-url origin https://github.com/nizarsd/noxim3d.git

# 2. Commit the changes
git commit -m "$CM"

# 3. Push to GitHub
git push origin main