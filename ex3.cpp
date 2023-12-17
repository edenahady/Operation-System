// Tomer Aligaev 208668129

/*
 * IMPORTANT!!! The program is working with the second option for configuration file that was shown
 * in the instructions.
 */

#include <iostream>
#include <semaphore.h>
#include <queue>
#include <mutex>
#include <fstream>
#include <unistd.h>

using namespace std;

//Global variables that will be used in the code.
#define DONE "DONE"
int NUM_OF_PRODUCERS;
int FINAL_QUEUE_SIZE;


/*
 * Helper class for passing info about a producer
 */
class ProducerInfo {
public:
    int id;
    int numberOfArticles;
    int queueSize;

    ProducerInfo(int id, int numberOfArticles, int queueSize) {
        this->id = id;
        this->numberOfArticles = numberOfArticles;
        this->queueSize = queueSize;
    }
};

//Bounded queue based on the example shown in class.
class BoundedQueue {
private:
    int size; //size of the queue
    queue<string> q;
    sem_t empty; //counts the number of free places for new articles
    sem_t full; //counts the number of articles ready to be consumed
    pthread_mutex_t m;

public:
    BoundedQueue(int size) {
        this->size = size;
        //initializing the semaphores and mutex of the queue.
        if (sem_init(&empty, 0, size) != 0) {
            perror("Couldn't initialize semaphore");
            exit(-1);
        }
        if (sem_init(&full, 0, 0) != 0) {
            perror("Couldn't initialize semaphore");
            exit(-1);
        }
        if (pthread_mutex_init(&m, NULL) != 0) {
            perror("Couldn't initialize mutex");
            exit(-1);
        }

    }

    //Adding an element to the end of the queue
    void insert(string article) {
        //Checking if there is a place in the queue for the added element, if not we will wait
        if (sem_wait(&empty) != 0) {
            perror("Couldn't decrement empty semaphore");
            exit(-1);
        }
        if (pthread_mutex_lock(&m) != 0) {
            perror("Couldn't lock mutex");
            exit(-1);
        }
        q.push(article);
        if (pthread_mutex_unlock(&m) != 0) {
            perror("Couldn't unlock mutex");
            exit(-1);
        }
        //Updating that an article was added.
        if (sem_post(&full) != 0) {
            perror("Couldn't increment full semaphore");
            exit(-1);
        }
    }

    //Removing the element at the top of the queue.
    string remove() {
        //Checking if there is an article in the queue , if not we will wait.
        if (sem_wait(&full) != 0) {
            perror("Couldn't decrement full semaphore");
            exit(-1);
        }
        if (pthread_mutex_lock(&m) != 0) {
            perror("Couldn't lock mutex");
            exit(-1);
        }
        string article = q.front();
        q.pop();
        if (pthread_mutex_unlock(&m) != 0) {
            perror("Couldn't unlock mutex");
            exit(-1);
        }
        //Updating that an article was consumed.
        if (sem_post(&empty) != 0) {
            perror("Couldn't increment full semaphore");
            exit(-1);
        }
        return article;
    }

    ~BoundedQueue() {
        sem_destroy(&empty);
        sem_destroy(&full);
        pthread_mutex_destroy(&m);
    }

};

//Unbounded queue based on the example shown in class.
class UnBoundedQueue {
private:
    queue<string> q;
    sem_t full; //counts the number of articles ready to be consumed
    pthread_mutex_t m;

public:
    UnBoundedQueue() {
        //initializing the semaphore and mutex of the queue.
        if (sem_init(&full, 0, 0) != 0) {
            perror("Couldn't initialize semaphore");
            exit(-1);
        }
        if (pthread_mutex_init(&m, NULL) != 0) {
            perror("Couldn't initialize mutex");
            exit(-1);
        }

    }

    //Adding an element to the end of the queue
    void insert(string article) {
        if (pthread_mutex_lock(&m) != 0) {
            perror("Couldn't lock mutex");
            exit(-1);
        }
        q.push(article);
        if (pthread_mutex_unlock(&m) != 0) {
            perror("Couldn't unlock mutex");
            exit(-1);
        }
        //Updating that an article was added.
        if (sem_post(&full) != 0) {
            perror("Couldn't increment full semaphore");
            exit(-1);
        }
    }

    //Removing the element at the top of the queue.
    string remove() {
        //Checking if there is an article in the queue , if not we will wait.
        if (sem_wait(&full) != 0) {
            perror("Couldn't decrement full semaphore");
            exit(-1);
        }
        if (pthread_mutex_lock(&m) != 0) {
            perror("Couldn't lock mutex");
            exit(-1);
        }
        string article = q.front();
        q.pop();
        if (pthread_mutex_unlock(&m) != 0) {
            perror("Couldn't unlock mutex");
            exit(-1);
        }
        return article;
    }

    ~UnBoundedQueue() {
        sem_destroy(&full);
        pthread_mutex_destroy(&m);
    }
};


//Vector of all the queues of all the producers.
vector<BoundedQueue> PRODUCER_QUEUES;
//Vector for the queues of the news,sports and weather co-editors.
vector<UnBoundedQueue> TOPIC_QUEUES = vector<UnBoundedQueue>(3, UnBoundedQueue());
//The queue of articles which is used by the screen manager.
BoundedQueue FINAL_QUEUE = BoundedQueue(0);


//Producing new articles in 3 different topics.
void *producer(void *arg) {
    ProducerInfo info = *(ProducerInfo *) arg;
    int newsCounter = 0, sportsCounter = 0, weatherCounter = 0;
    //Creating all the articles of the producers in 3 different topics.
    for (int i = 0; i < info.numberOfArticles; ++i) {
        int articleType = i % 3;
        string begin = "Producer " + to_string(info.id);
        string article;
        if (articleType == 0) {
            article = begin + " SPORTS " + to_string(sportsCounter);
            ++sportsCounter;
        } else if (articleType == 1) {
            article = begin + " NEWS " + to_string(newsCounter);
            ++newsCounter;
        } else if (articleType == 2) {
            article = begin + " WEATHER " + to_string(weatherCounter);
            ++weatherCounter;
        }
        //Adding the article to the queue of the producer.
        PRODUCER_QUEUES.at(info.id - 1).insert(article);
    }
    PRODUCER_QUEUES.at(info.id - 1).insert(DONE);
    pthread_exit(NULL);
}

//Receive articles from the produces and sorts them by topic.
void *dispatcher(void *arg) {
    bool shouldStop;

    /*
     * The loop will run while not all producers are done producing articles. If all producres are done then all
     * their queues will have "DONE" string at their top. at such situation "doneCount == NUM_OF_PRODUCERS" will be
     * true ant the loop will break.
     */
    while (shouldStop != true) {
        int doneCount = 0;
        shouldStop = false;
        for (int i = 0; i < NUM_OF_PRODUCERS; ++i) {
            string article = PRODUCER_QUEUES.at(i).remove();
            if (article.compare(DONE) == 0) {
                //If the producer is done we count it.
                PRODUCER_QUEUES.at(i).insert(DONE);
                ++doneCount;
            }
                //Adding the article to the queue with the corresponding topic.
            else if (article.find("SPORTS") != string::npos) {
                TOPIC_QUEUES[0].insert(article);
            } else if (article.find("NEWS") != string::npos) {
                TOPIC_QUEUES[1].insert(article);
            } else if (article.find("WEATHER") != string::npos) {
                TOPIC_QUEUES[2].insert(article);
            }
        }
        if (doneCount == NUM_OF_PRODUCERS) {
            shouldStop = true;
        }
    }
    //Adding "DONE" string to each topic-queue to indicate that no articles will be added.
    for (int i = 0; i < TOPIC_QUEUES.size(); ++i) {
        TOPIC_QUEUES[i].insert(DONE);

    }
    pthread_exit(NULL);
}

//Editing all articles of a certain topic, the topic number is indicated in the passed argument.
void *coEditor(void *arg) {
    string article = "";
    int numberOfQ = *(int *) arg;
    do {
        article = TOPIC_QUEUES[numberOfQ].remove();
        //Putting back the "DONE" string so the "DONE" indication won't be lost.
        if (article.compare(DONE) == 0)
            TOPIC_QUEUES[numberOfQ].insert(DONE);
        else {
            //"editing" the article for 0.1 second.
            usleep(100000);
            FINAL_QUEUE.insert(article);
        }
    } while (article.compare(DONE) != 0);
    //Adding "DONE" string to the end of the screen manager queue to indicate that the co-editor is done editing.
    FINAL_QUEUE.insert(DONE);
    pthread_exit(NULL);
}

//Printing all articles to the screen.
void *screenManager(void *arg) {
    int doneCount = 0;

    /*
     * Publishing articles to the screen while there are still articles. After the screen manager encountered 3
     * "DONE" articles it means that all the articles in all 3 topics were published.
     */
    while (doneCount != 3) {
        string article = FINAL_QUEUE.remove();
        if (article.compare(DONE) == 0) {
            ++doneCount;
        } else
            cout << article << endl;
    }
    cout << DONE << endl;
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    string str;
    string firstArg;
    string numberOfArticles;
    string queueSize;
    vector<ProducerInfo> producersInfo;
    if (argc <= 1) {
        cout << "Please enter a path to the configuration file" << endl;
        return -1;
    }
    ifstream configFile(argv[1]);
    //Getting the information from the configuration file.
    while (getline(configFile, str)) {
        firstArg = str;

        /*
         * If the line after the firstArg line is not null it means that it is information about an editor. Else,
         * it is the size of the screen manager queue.
         */
        if (getline(configFile, numberOfArticles)) {
            getline(configFile, queueSize);
            producersInfo.push_back(ProducerInfo(stoi(firstArg), stoi(numberOfArticles), stoi(queueSize)));
            getline(configFile, str);

        } else {
            FINAL_QUEUE_SIZE = stoi(firstArg);
        }
    }
    //Initializing bounded queues for all producers. Also Initializing the screen manager with correct size.
    NUM_OF_PRODUCERS = producersInfo.size();
    PRODUCER_QUEUES = vector<BoundedQueue>(NUM_OF_PRODUCERS, BoundedQueue(0));
    FINAL_QUEUE = BoundedQueue(FINAL_QUEUE_SIZE);
    for (int i = 0; i < NUM_OF_PRODUCERS; ++i) {
        PRODUCER_QUEUES.at(i) = BoundedQueue(producersInfo.at(i).queueSize);
    }

    //Creating separate thread for each producer.
    pthread_t producersThread[NUM_OF_PRODUCERS];
    for (int i = 0; i < NUM_OF_PRODUCERS; ++i) {
        pthread_create(&producersThread[i], NULL, producer, (void *) &(producersInfo.at(i)));
    }

    //Creating separate thread for the dispatcher.
    pthread_t dispatcherThread;
    pthread_create(&dispatcherThread, NULL, dispatcher, NULL);

    //Creating separate threads for the 3 co-editor of each topic.
    pthread_t coEditorsThread0, coEditorsThread1, coEditorsThread2;
    int sports = 0, news = 1, weather = 2;
    pthread_create(&coEditorsThread0, NULL, coEditor, (void *) &sports);
    pthread_create(&coEditorsThread1, NULL, coEditor, (void *) &news);
    pthread_create(&coEditorsThread2, NULL, coEditor, (void *) &weather);

    //Creating separate threads for the screen manager. After it done all articles are printed and we're done.
    pthread_t screenManagerThread;
    pthread_create(&screenManagerThread, NULL, screenManager, NULL);
    void *returnVal;
    for (int i = 0; i < NUM_OF_PRODUCERS; ++i) {
        pthread_join(producersThread[i], &returnVal);
    }
    pthread_join(dispatcherThread, &returnVal);
    pthread_join(coEditorsThread0, &returnVal);
    pthread_join(coEditorsThread1, &returnVal);
    pthread_join(coEditorsThread2, &returnVal);
    pthread_join(screenManagerThread, &returnVal);
}