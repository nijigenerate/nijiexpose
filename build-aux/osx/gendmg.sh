DMGTITLE="Install nijiexpose"
DMGFILENAME="Install_Inochi_nijiexpose.dmg"

if [ -d "out/nijiexpose.app" ]; then
    if [ -f "out/$DMGFILENAME" ]; then
        echo "Removing prior install dmg..."
        rm "out/$DMGFILENAME"
    fi

    PREVPWD=$PWD
    cd out/
    echo "Building $DMGFILENAME..."

    # Create Install Volume directory

    if [ -d "InstallVolume" ]; then
        echo "Cleaning up old install volume..."
        rm -r InstallVolume
    fi

    mkdir -p InstallVolume
    cp ../LICENSE LICENSE
    cp -r "nijiexpose.app" "InstallVolume/nijiexpose.app"
    
    create-dmg \
        --volname "$DMGTITLE" \
        --volicon "Inochinijiexpose.icns" \
        --background "../build-aux/osx/dmgbg.png" \
        --window-size 800 600 \
        --icon "nijiexpose.app" 200 250 \
        --hide-extension "nijiexpose.app" \
        --eula "LICENSE" \
        --app-drop-link 600 250 \
        "$DMGFILENAME" InstallVolume/

    echo "Done! Cleaning up temporaries..."
    rm LICENSE

    echo "DMG generated as $PWD/$DMGFILENAME"
    cd $PREVPWD
else
    echo "Could not find nijiexpose for packaging..."
fi