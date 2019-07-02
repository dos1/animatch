# animatch

Animatch is an animal matching game.

To build and install animatch;

```
1. git clone https://gitlab.com/HolyPangolin/animatch
2. git submodule update --init --recursive
3. cd flatpak;
4.flatpak-builder --force-clean --repo repo-flatpak build-dir com.holypangolin.Animatch.json --install --user
```

To run animatch after building the flatpak, issue this command;

```
flatpak run com.holypangolin.Animatch
```
