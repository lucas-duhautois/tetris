#include <iostream>
#include <vector>
#include <array>
#include <cstdlib>
#include <SFML/Graphics.hpp>
#include <unistd.h> 
#include "time.h"

// Controls
sf::Keyboard::Key rotating = sf::Keyboard::Z; // tourner la pièce
sf::Keyboard::Key left = sf::Keyboard::Q; // bouger la pièce à gauche
sf::Keyboard::Key down = sf::Keyboard::S; // faire tomber la pièce
sf::Keyboard::Key right = sf::Keyboard::D; // bouger la pièce à droite
sf::Keyboard::Key pausing = sf::Keyboard::Escape; // pause

// Settings
const int resize = 1; // facteur de changement de taille de la fenêtre (pour une fenêtre 2 fois plus grande, resize = 2)
const int seed = 42; // changer la seed pour changer l'ordre d'apparition des pièces
int dt = 100000; // délai entre 2 actions (en microsecondes) ; multiplier par 10 pour avoir le délai initial entre 2 chutes de la pièce
const int inverseDifficulty = 5; // nombre de lignes nécessaires à réaliser pour diminuer dt de 10%

sf::RenderWindow window(sf::VideoMode(634*resize, 387*resize), "Validation SFML");

class board {
  protected :
  const int nx_; // dimension horizontale
  const int ny_; // dimension verticale
  std::vector<sf::Color> bg_; // couleur de la case (sf::Color::Black si la case n'est pas occupée)

  public :
  board()=default;
  board (const int nx, const int ny) : nx_{nx}, ny_{ny}, bg_(nx*ny, sf::Color::Black)
  {
  }

  std::pair<int, int> dim () {
    // permet d'accéder aux dimensions de la grille
    return std::make_pair(nx_, ny_);
  }

  int index (int x, int y) {
    // conversion des coordonnées de la case (x de gauche à droite, y de haut en bas) en son index dans bg_
    return y * nx_ + x;
  }

  bool test (int x, int y) {
    // test si la case aux coordonnées données est occupée
    if (bg_[index(x, y)] == sf::Color::Black) {
        return (false);
    }
    else {
        return (true);
    }
  }

  void add (int x, int y, sf::Color color) {
    // remplit la case aux coordonnées données avec la couleur donnée
    bg_[index(x,y)] = color;
  }

  void remove (int x, int y) {
    // libère la case aux coordonnées données
    bg_[index(x,y)] = sf::Color::Black;
  }

  int line () {
    // libère la ou les lignes remplies, fait tomber les autres cases et renvoie la valeur à ajouter au score
    int dscore = 0; // incrément de score à renvoyer
    int x; // numéro de la ligne en cours de vérification
    bool test; // état de la case en cours de vérification
    for (int y = ny_-1; y>=0; y--) { // on parcourt la grille de bas en haut (sens décroissant de y)
        x = 0;
        test = true;
        while (test) {
            test = this->test(x, y);
            x++;
            if (x == nx_+1) { // si on arrive en bout de ligne c'est qu'elle est remplie
                dscore++;
                for (int y2 = y; y2>0; y2--) { // fait tomber les autres cases
                    for (int x2 = 0; x2<nx_; x2++) {
                        bg_[index(x2,y2)] = bg_[index(x2,y2-1)];
                    }
                }
                x = 0;
            } // on reprend la vérification sur la même ligne car on peut avoir 2 lignes remplies consécutives
        }
    }
    return dscore;
  }

  void print() {
    // affiche la grille dans la fenêtre graphique sfml
    int dx = 17*resize; // taille horizontale d'une case (en pixels)
    int dy = 17*resize; // taille verticale d'une case (en pixels)
    sf::RectangleShape rectangle(sf::Vector2f(dx,dy));
    for (int y = 0; y < ny_; y++){
        for (int x = 0; x < nx_; x++){
            if (test(x, y)) {
                rectangle.setFillColor(bg_[index(x, y)]);
                rectangle.setPosition(227*resize+x*(dx+resize),11*resize+y*(dy+resize));
                window.draw(rectangle);
            }
        }
    }
  }

};

class piece {
    protected :
    std::pair<int, int> center_; // coordonnées du centre de la pièce
    std::array<int, 4> size_; // taille de la pièce par rapport au centre dans chaque direction, respectivement à droite (x+), en bas (y+), à gauche (x-), en haut (y-)
    std::vector<std::pair<int,int>> shape_; // coordonnées des cases de la pièce relativement au centre
    sf::Color color_; // couleur de la pièce

    virtual void other_rotate () = 0;

    public :
    piece() = default;
    piece (std::pair<int, int> center) : center_{center}, size_{{0,0,0,0}}, shape_{std::make_pair(0,0)} {}
    ~piece() = default;

    void fall () {
        // fait tomber la pièce d'une case
        center_.second++;
    }

    void up () {
        // fait monter la pièce d'une case
        center_.second--;
    }

    bool verify (board &game) {
        // vérifie si la pièce est en bas de la grille ou en collision avec une autre pièce
        auto dim = game.dim();
        if (center_.second + size_[1] >= dim.second) { // si la pièce est en bas de la grille 
            return true;
        }
        for (auto &c : shape_) { // si la pièce est en collision
            if (game.test(center_.first + c.first, center_.second + c.second)) {
                return true;
            }
        }
        return false;
    }

    void move_left (board &game) {
        // fait bouger horizontalement la pièce vers la gauche
        if (center_.first - size_[2] > 0) { // si possible dans les dimensions de la grille
            center_.first--;
            if (verify(game)) { // vérifie si le mouvement est possible
                center_.first++; // sinon annule le dernier mouvement illégal
            }
        }
    }

    void move_right (board &game) {
        // fait bouger horizontalement la pièce vers la droite
        if (center_.first + size_[0] < game.dim().first-1) { // si possible dans les dimensions de la grille
            center_.first++;
            if (verify(game)) { // vérifie si le mouvement est possible
                center_.first--; // sinon annule le dernier mouvement illégal
            }
        }
    }

    void rotate (board &game) {
        // fait tourner la pièce de 90° dans le sens horaire
        if ((center_.first-size_[1] >= 0) & (center_.first+size_[3] < game.dim().first)) {
            // tourne les dimensions de la pièce
            int temp = size_[3];
            size_[3] = size_[2];
            size_[2] = size_[1];
            size_[1] = size_[0];
            size_[0] = temp;
            other_rotate(); // rotation différente selon le type de pièce
        }
    }

    std::vector<std::pair<int,int>> all_positions () {
        // renvoie les coordonées de toutes les cases de la pièce dans la grille
        std::vector<std::pair<int,int>> result;
        for (auto &c : shape_) {
            result.push_back(std::make_pair(center_.first + c.first, center_.second + c.second));
        }
        return result;
    }

    void fastFall (board &game) {
        // chute rapide de la pièce
        while (!(verify(game))) {
            fall();
        }
        up(); // annule le dernier mouvement illégal
    }

    void add (board &game) {
        // ajoute la pièce au jeu
        for (auto &c : shape_) {
            game.add(center_.first + c.first, center_.second + c.second, color_);
        }
    }

    void remove (board &game) {
        // enlève la pièce du jeu
        for (auto &c : shape_) {
            game.remove(center_.first + c.first, center_.second + c.second);
        }
    }

    void printNext (sf::RenderWindow &window) {
        // affiche la pièce dans la fenêtre Next
        int dx = 17*resize; // taille horizontale d'une case (en pixels)
        int dy = 17*resize; // taille verticale d'une case (en pixels)
        sf::RectangleShape rectangle(sf::Vector2f(dx,dy));
        for (auto &c : shape_){
            rectangle.setFillColor(color_);
            rectangle.setPosition(524*resize+c.first*dx, 50*resize+c.second*dy);
            window.draw(rectangle);
        }
    }
};

class I : public piece {
    public :
    I (std::pair<int, int> center) : piece(center) {
        color_ = sf::Color::Yellow;
        size_[1] = 2;
        size_[3] = 1;
        shape_.push_back(std::make_pair(0,-1));
        shape_.push_back(std::make_pair(0,1));
        shape_.push_back(std::make_pair(0,2));
    }
    ~I() = default;

    void other_rotate () {
        for (auto &c : shape_){
            int temp = c.first;
            c.first = c.second;
            c.second = temp;
        }
        if (size_[0] == 2) {
            center_.first++;
        }
        else if (size_[1] == 2){
            center_.second++;
            for (auto &c : shape_){
                c.first--;
            }
        }
        else if (size_[2] == 2){
            center_.first--;
        }
        else {
            center_.second--;
            for (auto &c : shape_){
                c.first++;
            }
        }
    }
}; 

class T : public piece {
    public :
    T (std::pair<int, int> center) : piece(center) {
        color_ = sf::Color::Blue;
        size_[0] = 1;
        size_[1] = 1;
        size_[3] = 1;
        shape_.push_back(std::make_pair(0, -1));
        shape_.push_back(std::make_pair(1, 0));
        shape_.push_back(std::make_pair(0, 1));
    }
    ~T() = default;

    void other_rotate () {
        if (size_[3] == 0) {
            shape_[1] = std::make_pair(-1, 0);
        }
        else if (size_[0] == 0) {
            shape_[2] = std::make_pair(0, -1);
        }
        else if (size_[1] == 0) {
            shape_[3] = std::make_pair(1, 0);
        }
        else {
            shape_[1] = std::make_pair(0, -1);
            shape_[2] = std::make_pair(1, 0);
            shape_[3] = std::make_pair(0, 1);
        }
    }
};

class O : public piece {
    public :
    O (std::pair<int, int> position) : piece(position) {
        color_ = sf::Color::Green;
        size_[0] = 1;
        size_[1] = 1;
        shape_.push_back(std::make_pair(0, 1));
        shape_.push_back(std::make_pair(1, 0));
        shape_.push_back(std::make_pair(1, 1));
    }
    ~O() = default;

    void other_rotate () {
        int temp = size_[0];
        size_[0] = size_[1];
        size_[1] = size_[2];
        size_[2] = size_[3];
        size_[3] = temp;
    }
};

class L : public piece {
    public :
    L (std::pair<int, int> position) : piece(position) {
        color_ = sf::Color::Cyan;
        size_[0] = 1;
        size_[1] = 1;
        size_[3] = 1;
        shape_.push_back(std::make_pair(0, -1));
        shape_.push_back(std::make_pair(0, 1));
        shape_.push_back(std::make_pair(1, 1));
    }
    ~L() = default;

    void other_rotate () {
        for (int i = 1; i < 3; i++) {
            auto temp = shape_[i].first;
            shape_[i].first = shape_[i].second;
            shape_[i].second  = temp;
        }
        if (size_[3] == 0) {
            shape_[3].first = -1;
        }
        else if (size_[0] == 0) {
            shape_[3].second = -1;
        }
        else if (size_[1] == 0) {
            shape_[3].first = 1;
        }
        else {
            shape_[3].second = 1;
        }
    }
};

class J : public piece {
    public :
    J (std::pair<int, int> position) : piece(position) {
        color_ = sf::Color::Magenta;
        size_[1] = 1;
        size_[2] = 1;
        size_[3] = 1;
        shape_.push_back(std::make_pair(0, -1));
        shape_.push_back(std::make_pair(0, 1));
        shape_.push_back(std::make_pair(-1, 1));
    }
    ~J() = default;

    void other_rotate () {
        for (int i = 1; i < 3; i++) {
            auto temp = shape_[i].first;
            shape_[i].first = shape_[i].second;
            shape_[i].second  = temp;
        }
        if (size_[3] == 0) {
            shape_[3].second = 1;
        }
        else if (size_[0] == 0) {
            shape_[3].first = -1;
        }
        else if (size_[1] == 0) {
            shape_[3].second = -1;
        }
        else {
            shape_[3].first = 1;
        }
    }
};

class Z : public piece {
    public :
    Z (std::pair<int, int> position) : piece(position) {
        color_ = sf::Color::Red;
        size_[0] = 1;
        size_[1] = 1;
        size_[3] = 1;
        shape_.push_back(std::make_pair(1, -1));
        shape_.push_back(std::make_pair(1, 0));
        shape_.push_back(std::make_pair(0, 1));
    }
    ~Z() = default;

    void other_rotate () {
        if (size_[3] == 0) {
            shape_[1].second = 1;
            shape_[2].first = -1;
        }
        else if (size_[0] == 0) {
            shape_[1].first = -1;
            shape_[3].second = -1;
        }
        else if (size_[1] == 0) {
            shape_[1].second = -1;
            shape_[2].first = 1;
        }
        else {
            shape_[1].first = 1;
            shape_[3].second = 1;
        }
    }
};

class S : public piece {
    public :
    S (std::pair<int, int> position) : piece(position) {
        color_ = sf::Color::White;
        size_[0] = 1;
        size_[1] = 1;
        size_[3] = 1;
        shape_.push_back(std::make_pair(0, -1));
        shape_.push_back(std::make_pair(1, 0));
        shape_.push_back(std::make_pair(1, 1));
    }
    ~S() = default;

    void other_rotate () {
        if (size_[3] == 0) {
            shape_[1].second = 1;
            shape_[3].first = -1;
        }
        else if (size_[0] == 0) {
            shape_[2].first = -1;
            shape_[3].second = -1;
        }
        else if (size_[1] == 0) {
            shape_[1].second = -1;
            shape_[3].first = 1;
        }
        else {
            shape_[2].first = 1;
            shape_[3].second = 1;
        }
    }
};

piece *generation(int alea, int middlex) {
    // génère une pièce dont la forme dépend de la valeur alea, au milieu en haut de la grille
    if (alea == 0) {
        I *tetromino = new I (std::make_pair(middlex, 1));
        return (tetromino);
    }
    else if (alea == 1) {
        T *tetromino = new T (std::make_pair(middlex, 1));
        return (tetromino);
    }
    else if (alea == 2) {
        O *tetromino = new O (std::make_pair(middlex, 0));
        return (tetromino);
    }
    else if (alea == 3) {
        L *tetromino = new L (std::make_pair(middlex, 1));
        return (tetromino);
    }
    else if (alea == 4) {
        J *tetromino = new J (std::make_pair(middlex, 1));
        return (tetromino);
    }
    else if (alea == 5) {
        Z *tetromino = new Z (std::make_pair(middlex, 1));
        return (tetromino);
    }
    else {
        S *tetromino = new S (std::make_pair(middlex, 1));
        return (tetromino);
    }
}

int main() {
    // initialisation des variables du jeu
    const int nx = 10; // dimension horizontale
    const int ny = 20; // dimension verticale

    srand(seed);
    int alea = rand() % 7; // génère un entier pseudo-aléatoire entre 0 et 6 inclus

    int i = 0; // numéro de la pièce en cours

    int counter = 0; // permet de faire tomber les pièces plus lentement que la fréquence de rotation/déplacement

    int score = 0;
    int dscore; // incrément de score suite à la suppression de ligne(s)
    int Dscore = 0; // incrément de score depuis la dernière accélération du jeu

    bool gameOver = false;
    bool pause = false;

    // Importation des images et font
    sf::Texture background;
    background.loadFromFile("tetris_bg.png");
    sf::Sprite sprite_background;
    sprite_background.setPosition(0,0);
    sprite_background.setTexture(background);
    sprite_background.setScale(resize, resize);

    sf::Font font;
    font.loadFromFile("Carre-G6Vq.otf");
    sf::Text printScore;
    printScore.setFont(font);
    printScore.setFillColor(sf::Color::White);
    printScore.setPosition(54*resize, 34*resize);
    printScore.setCharacterSize(30);

    sf::Texture pauseScreen;
    pauseScreen.loadFromFile("tetris_pause.jpg");
    sf::Sprite sprite_pauseScreen;
    sprite_pauseScreen.setPosition(150*resize, 110*resize);
    sprite_pauseScreen.setTexture(pauseScreen);
    sprite_pauseScreen.setScale(resize*0.5, resize*0.5);

    sf::Texture gameOverScreen;
    gameOverScreen.loadFromFile("tetris_game_over.jpg");
    sf::Sprite sprite_gameOverScreen;
    sprite_gameOverScreen.setPosition(50*resize, 110*resize);
    sprite_gameOverScreen.setTexture(gameOverScreen);
    sprite_gameOverScreen.setScale(resize*0.5, resize*0.5);

    board game(nx, ny);

    int middlex = game.dim().first/2; // milieu hotizontal de la grille

    piece* tetromino = generation(alea, middlex);

    alea = rand() % 7;
    piece* next_tetromino = generation(alea, middlex); // génère une pièce en avance pour pouvoir afficher la pièce suivante

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        window.clear(sf::Color::Black);

        // Affichage
        window.draw(sprite_background);
        game.print();
        printScore.setString(std::to_string(score));
        window.draw(printScore);
        next_tetromino->printNext(window);

        if (!(gameOver)) {

            tetromino->remove(game);

            if (sf::Keyboard::isKeyPressed(rotating)){
                tetromino->rotate(game);
                usleep(dt); // évite de tourner plusieurs fois
            }
            else if (sf::Keyboard::isKeyPressed(left)){
                tetromino->move_left(game);
                usleep(dt); // évite de bouger plusieurs fois
            }
            else if (sf::Keyboard::isKeyPressed(right)){
                tetromino->move_right(game);
                usleep(dt); // évite de bouger plusieurs fois
            }
            else if (sf::Keyboard::isKeyPressed(down)){
                tetromino->fastFall(game);
                usleep(dt); // évite de faire tomber plusieurs pièces de suite
            }
            else if (sf::Keyboard::isKeyPressed(pausing)){
                pause = true;
                window.draw(sprite_pauseScreen);
                window.display();
                usleep(dt*2); // évite de sortir directement de la pause
                while (pause) {
                    if (sf::Keyboard::isKeyPressed(pausing)){
                        pause = false;
                    }
                    usleep(dt); // évite de retourner directement en pause
                }
                window.clear(sf::Color::Black);
                window.draw(sprite_background);
                game.print();
                printScore.setString(std::to_string(score));
                window.draw(printScore);
                window.display();
            }
        
            if (counter >= 10) { // fait tomber la pièce naturellement une fois sur 10
                tetromino->fall();
                if (tetromino->verify(game)) {
                    tetromino->up(); // corrige le dernier mouvement illégal du tetromino
                    tetromino->add(game);
                    tetromino = next_tetromino;
                    alea = rand() % 7;
                    next_tetromino = generation(alea, middlex);
                    dscore = game.line(); // supprime les lignes remplies et incrémente le score de manière idoine
                    score = score + dscore;
                    Dscore = Dscore + dscore;
                    if (tetromino->verify(game)) { // on perd si la nouvelle pièce est initialement en collision
                        gameOver = true;
                    }
                }
                counter = 0;
            }


            tetromino->add(game);

            if (Dscore >= inverseDifficulty) { // accélère le jeu toutes les 5 lignes supprimées (peut être changé par le paramètre difficulty)
                Dscore = 0;
                dt = dt*0.9;
            }

            counter++;

        }
        else {
            window.draw(sprite_gameOverScreen);
        }

        usleep(dt);

        window.display();
    }

    return EXIT_SUCCESS;

}