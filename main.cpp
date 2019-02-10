#include <vector>
#include <algorithm>
#include <iostream>
#include <random>
#include <SFML/Graphics.hpp>
#include <thread>
#include <numeric>
//#include <future>
#include <cmath>
//#include <mutex>
#include <chrono>
#include <iomanip>

#include <parallel/algorithm>

#include "Particle.h"
#include "StatDisplay/StatDisplay.h"
#include "StatDisplay/FPS.h"

#include "Quadtree.h"

const float VECTOR_SPEED_MULTIPLIER = 1, GRAVITY_GLOBAL_VARIABLE = 0.2;
int GRAVITY_ENABLED = 0, DRAWQT = 0;

bool vecInUse = false, mouseButtonIsPressed = false, rightButton = false;
std::vector<Particle> particles;
sf::Vector2i mouseLocation = sf::Vector2i(0,0);
unsigned int width = 1000, height = 800;
sf::FloatRect border{-(float)width,(float)height-100,(float)width * 3,100};
Quadtree<Particle> quadtree( -10.0f, -10.0f, width + 20, height + 20, 0, 500, 8);

int threadCount = std::thread::hardware_concurrency();
std::vector<std::vector<Particle *>> subVectors;

struct threadObj
{
    std::vector<Particle *> particles;
    std::thread th;
    bool done;
};
std::vector<threadObj *> threads;

void initializeThreading()
{
    threads.reserve(threadCount);
    for (int i = 0; i < threadCount; ++i)
    {
        std::vector<Particle*> temp;
        temp.reserve(50);
        subVectors.push_back(temp);
    }
}

void fillThreadVectors(std::vector<Particle> &input)
{
    int elements = floor(input.size() / threadCount), indexinInputVec = 0;
    std::cout << "elements = " << elements << std::endl;
    for (int i = 0; i < threadCount; ++i)
    {
        std::vector<Particle *> temp;
        if(i < threadCount-1)
        {
            for (int j = 0; j < elements; ++j)
            {
                temp.push_back(&input[j+(i*elements)]);
                indexinInputVec++;
            }
        } else
        {
            for (int k = indexinInputVec; k < input.size(); ++k)
            {
                temp.push_back(&input[k]);
            }
        }
        subVectors[i] = temp;
    }
}


void createParticles(int ammount, float sizeMultiplier)
{
    if (vecInUse)
        return;

    vecInUse = true;
    particles.clear();

    for (int i = 0; i < ammount; ++i)
    {
        Particle par(
                rand() % width + 1,
                rand() % height + 1 -100,
                (rand() % 10 + 1) * sizeMultiplier);

        par.setColor(rand() % 255 + 1, rand() % 255 + 1, rand() % 255 + 1);

        float rng = rand() % 1000 + 1;
        if (rng < 800)
            rng += 200;
        par.id = i;
        par.setSpeed(rng / 1000);
        particles.push_back(par);


    }
    fillThreadVectors(particles);
    vecInUse = false;
}



//int collisionCheckCount = 0, maxCollisionsOnRound = 0;
void updateParticles(Particle *par)
{
    //sf::Vector2f axisLock(1,1);
    int intersectCount = 0;
    std::vector<Particle *> QTResult = quadtree.GetObjectsAt(par->x, par->y);

    /*for (auto &i : QTResult)
    {
        if (par->obj().getGlobalBounds().contains(i->location()))
        {
            intersectCount++;

            par->setColor(255,0,0);
            //collisionCheckCount++;
        }
        if (i->obj().getGlobalBounds().contains(par->location()))
        {
            i->setColor(255, 0, 0);
            intersectCount++;
        }

    }
    if (intersectCount < 1)
        par->setColor(255,255,255);
*/
    if (mouseButtonIsPressed) {
        int reverse = rightButton ? -1 : 1;
        // move the vector
        par->moveTowards(sf::Vector2f(mouseLocation), VECTOR_SPEED_MULTIPLIER * reverse);
    }

    par->prevLocation = par->location();
    par->update();


    /*if (collisionCheckCount > maxCollisionsOnRound)
        maxCollisionsOnRound = collisionCheckCount;
    std::cout << maxCollisionsOnRound << std::endl;
    collisionCheckCount = 0;*/

}


void updateThreads(bool *done, std::vector<Particle *> *input)
{
    std::cout << "Thread started:" << std::endl;
    while (true)
    {
        if (*done)
        {
            std::this_thread::sleep_for(3ms);
            continue;
        }
        for (auto &i : *input)
        {
            //std::cout << "Updating particles..." << std::endl;
            updateParticles(i);
        }

        *done = true;
    }
}


/*
 * Font Global variable and button structure
 * Don't optimize font global variable to be included in any function, and it's loaded as first line of main() for a reason
 * Struct button: Pretty self explanatory, contains all the data that a button needs to work
 */
sf::Font font_Glob;
struct button
{
    sf::RectangleShape rect;
    sf::FloatRect fRect;
    sf::Text text;
    std::vector<std::string> text_Vec;
    int *target, optionCount;
};

/*
 * Create button function:
 * Creating buttons is expensive DO NOT CREATE LOT OF BUTTONS
 * Gives interactable toggle buttons with simple function with few inputs with power to affect any boolean assainged to it.. unles const
 * You can change the button appearance in this function
 *
 */
std::vector<button> buttons;
sf::Vector2i buttonSize(150,30);
void createButton(int col, int row, std::vector<std::string> optionTexts, int& target)
{
    button button1;
    sf::Vector2f pos(buttonSize.x * row, height - buttonSize.y * col);
    button1.rect.setSize((sf::Vector2f)buttonSize);
    button1.rect.setFillColor(sf::Color::Green);
    button1.rect.setPosition(pos);
    button1.rect.setOutlineColor(sf::Color::Black);
    button1.rect.setOutlineThickness(2.5);
    button1.fRect = button1.rect.getGlobalBounds();
    button1.text.setFont(font_Glob);
    button1.text.setString(optionTexts[0]);
    button1.optionCount = optionTexts.size();
    button1.text_Vec = optionTexts;
    button1.text.setPosition(pos);
    button1.text.setCharacterSize(25);
    button1.text.setFillColor(sf::Color::Red);
    button1.target = &target;
    buttons.push_back(button1);

}

int desiredParticles = 10000;
void handleEvents(sf::Event *e, sf::Window *w)
{
    if (e->type == sf::Event::Closed)
        w->close();

    if (e->type == sf::Event::KeyPressed)
    {
        if (e->key.code == sf::Keyboard::Escape)
        {
            w->close();
        }
        if (e->key.code == sf::Keyboard::F5)
        {
            std::thread t1(createParticles, desiredParticles, 0.5);
            t1.detach();
        }

        if (e->key.code == sf::Keyboard::Return)
        {
            int largestNumber = 0;
            for (const auto &i : quadtree.GetLeafObjectsCount())
            {
                if (i > largestNumber)
                    largestNumber = i;
                std::cout << i << ", ";
            }
            std::cout << std::endl;
            std::cout << "Largest Number = " << largestNumber << std::endl;

        }
    }

    if (e->type == sf::Event::MouseButtonPressed || mouseButtonIsPressed)
        mouseLocation = sf::Mouse::getPosition(*w);

    if (e->type == sf::Event::MouseButtonPressed)
    {
        mouseButtonIsPressed = !mouseButtonIsPressed;
        rightButton = sf::Mouse::isButtonPressed(sf::Mouse::Right);
        //Seeing if the mouse is on any button
        for (auto &i : buttons)
        {
            if (i.fRect.contains((sf::Vector2f)mouseLocation))
            {
                //If mouse is on a button and was clicked
                std::cout << i.text_Vec[*i.target] << i.optionCount << std::endl;
                std::cout << *i.target << std::endl;
                if (*i.target < i.optionCount-1)
                    *i.target = *i.target + 1;
                else
                    *i.target = 0;
                i.text.setString(i.text_Vec[*i.target]);

            }
        }

    } else if (e->type == sf::Event::MouseButtonReleased)
    {
        mouseButtonIsPressed = !mouseButtonIsPressed;
        rightButton = sf::Mouse::isButtonPressed(sf::Mouse::Right);

    }
}


int trailLength = 500;
std::vector<sf::Texture> frames;
int main()
{
    initializeThreading();
    std::cout << threadCount << std::endl;
    StatDisplay sd;

    FPS fps;
    sd.AddDisplay(&fps);

    sf::Text sdText;
    sdText.setFont(font_Glob);
    sdText.setFillColor(sf::Color::Green);



    int bgColor = 0;
    font_Glob.loadFromFile("arial.ttf");
    sf::RenderWindow window(sf::VideoMode(width, height), "lolkek!");
    window.setFramerateLimit(144);

    createParticles(64, 1);


    std::vector<std::string> options1{"BGColor B", "BGColor W"};
    createButton(1,3,options1,bgColor);
    createButton(1,2,std::vector<std::string>{"Gravity OFF", "Gravity ON"}, GRAVITY_ENABLED);
    createButton(1,4,std::vector<std::string>{"Hide QT", "Draw QT"}, DRAWQT);

    for (int l = 0; l < threadCount; ++l)
    {
        threadObj *thread = new threadObj;
        thread->done = false;
        thread->th = std::thread(updateThreads, &thread->done, &subVectors[l]);
        //thread->th.detach();
        threads.push_back(thread);
    }

    frames.reserve(trailLength);


    bool first(true);
    while (window.isOpen())
    {
        window.display();

        sf::Event event;
        while (window.pollEvent(event))
        {
            handleEvents(&event, &window);
        }

        window.clear(bgColor ? sf::Color::White : sf::Color::Black);

        if (vecInUse)
            continue;

        quadtree.Clear();
        for (auto &k : particles)
        {
            quadtree.AddObject( &k );
        }

       for (const auto &l : threads)
        {
            l->done = false;
        }

        int loopcount = 0;
        while (true)
        {
            std::this_thread::sleep_for(1ms);
            loopcount++;
            unsigned int completedThreads = 0;
            for (const auto &i : threads)
                if (i->done)
                    completedThreads++;

            if (completedThreads >= threadCount)
                break;
            if (loopcount > 1000)
            {
                std::cout << "Infinite Loop" << std::endl;
                window.close();
                break;
            }
            
        }

        for (const auto &m : subVectors)
        {
            for (const auto &i : m)
            {
                window.draw(i->obj());
            }
        }
        /*for (auto &i : particles)
        {
            //if (i.location() == i.prevLocation)
            //    std::cout << i.id << ", ";
            window.draw(i.obj());
            //std::cout << "(" << setfill('0') << setw(3) <<  floor(i.location().x) << "," << setfill('0') << setw(3) << floor(i.location().y) << ") ";
        }*/
        //std::cout << std::endl;

        for (const auto &j : buttons)
        {
            window.draw(j.rect);
            window.draw(j.text);
        }
        if (DRAWQT)
            quadtree.Draw(window);


        sdText.setString(sd.GetText());
        window.draw(sdText);


    }
    for (const auto &m : threads)
    {
        delete(m);
    }

    return 0;
}

