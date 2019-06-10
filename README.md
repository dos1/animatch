# animatch

Animatch is an animal matching game.

To build animatch;

1. git clone https://gitlab.com/HolyPangolin/animatch
2. git submodule update --init --recursive
3. cd flatpak;
4. sudo flatpak-builder --force-clean --repo repo-flatpak build-dir com.holypangolin.Animatch.json
