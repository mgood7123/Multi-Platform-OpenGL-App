if [[ $(basename "$(readlink -e "$(which python3)")") == "AppInstallerPythonRedirector.exe" ]]
	then
		echo "python3 has not been installed, please run 'python3' to install it from the Microsoft App Store"
		exit 1
else
  echo "python3 exists"
fi

if [[ ! -f .steps_synched_deps ]]
  then
    if [[ -d skia ]]
      then
        echo "deps sync incomplete, removing existing skia"
        rm -rf skia
    fi
fi

if [[ ! -d skia ]]
  then
    rm -rf .steps_*
    if [[ -d skia_backup ]]
      then
        touch .steps_cloned_skia
    fi
fi

if [[ ! -f .steps_copy_skia ]]
	then
    if [[ ! -f .steps_cloned_skia ]]
      then
        if [[ -d skia ]]
          then
              echo "clone incomplete, removing existing skia"
              rm -rf skia
          else
            echo "skia does not exist"
        fi
        git clone https://skia.googlesource.com/skia.git --depth=1 || exit 1
        mv skia skia_backup || exit 1
        echo "move [ skia -> skia_backup ]"
        touch .steps_cloned_skia
    fi
    echo "copying skia..."
		cp -R skia_backup skia || exit 1
    echo "copy [ skia_backup -> skia ]"
		touch .steps_copy_skia
    echo "copied skia"
fi

if [[ ! -f .steps_patched_deps ]]
	then
    echo "patching git-sync-deps..."
    cp git-sync-deps skia/tools/git-sync-deps
    cp git-sync-deps skia_backup/tools/git-sync-deps
    echo "patched git-sync-deps"
		touch .steps_patched_deps
fi

if [[ ! -f .steps_synched_deps ]]
	then
		echo "syncing deps..."
		cd skia || exit 1
    python3 tools/git-sync-deps || exit 1
		cd .. || exit 1
		echo "synced deps"
		touch .steps_synched_deps
fi

if [[ ! -f .steps_removed_backup ]]
  then
    echo "removing skia_backup..."
    rm -rf skia_backup || exit 1
    echo "removed skia_backup"
		touch .steps_removed_backup
fi

if [[ ! -f .steps_fetch_ninja ]]
	then
		echo "fetching ninja"
		cd skia || exit 1
		bin/fetch-ninja || exit 1
		cd .. || exit 1
		echo "fetched ninja"
		touch .steps_fetch_ninja
fi

if [[ ! -f .steps_fetch_gn ]]
	then
		echo "fetching gn"
		cd skia || exit 1
		bin/fetch-gn || exit 1
		cd .. || exit 1
		echo "fetched gn"
		touch .steps_fetch_gn
fi

exit 0
