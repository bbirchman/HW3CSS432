/**Created by Jayden Stipek and Duncan Spani 
* 12/2/2020
* CSS432 HW3
* Implementation file for the class that implements stopNwait and SlidingWindow
* Client and Server
**/
#include <iostream>
#include "udphw3.h"

#define TIMEOUT 1500
using namespace std;

/**
 * sends message[] and receives an acknowledgment from the server max
 * If the client cannot receive an acknowledgement immedietly
 * it should start a Timer. If a timeout occurs (i.e., no response after 1500 usec)
 * the client must resend the same message. The function must count the number of messages retransmitted and
 * return it to the main function as its return value.
 * @param socket the udp socket send data from
 * @param int the maximum number of time (messages) to send
 * @param message the message buffer
 * @return number of retransmissions
 **/
int clientStopWait( UdpSocket &sock, const int max, int message[] )
{
 
    cout << "beginning clientStopWait" << endl;
    int resubmissions = 0;
    for(int i = 0; i < max; i++){ // loop for the 20000 times
        message[0] = i; //insert the message into message[0]
        Timer timer;
        bool response = false;
        int sent;
        sent = sock.sendTo((char*) message, MSGSIZE); //send the message to the server
        cout << "data sent to server" << endl;
        timer.start();
        while(true) //while I have not receieved an response
        {
            int responseData = sock.pollRecvFrom(); //Any data been recieved?
            if(responseData > 0)
                break; //response is true and then cut the while loop

            if(timer.lap() > TIMEOUT && !response) //If we go over the 1500 Timeout 
            {
                cout << "response found " << endl;
                response = true;
                resubmissions++; //add to resubmissions count
                i--; //take away from the for loop total     
                break;
            }
        }
        if(response) //if the response is found dont recieve the message
            continue; //skip the iteration

        sock.recvFrom((char*) message, MSGSIZE); //receieve the message from ther server
        if(message[0] != i) //If it is a bad message
        {
            i--;    
            resubmissions++;
            continue; //Restart
        }
    }
    cout << "finishing" << endl;
    return resubmissions;
}

/**
 * repeats receiving message[] and sending an acknowledgment at a server side max (=20,000) times using the sock object.
 * @param socket the udp socket send data to
 * @param int the maximum number of time (messages) to receive
 * @param message the message buffer to receive 
**/
void serverReliable( UdpSocket &sock, const int max, int message[] )
{
    cout << "inside serverReliable" << endl;    
    for(int i = 0; i < max; i++)    //loop through the 20000 messages
    {
        while(true)
        {
            int recievedData = 0;
            recievedData = sock.pollRecvFrom(); //Any data been recieved?
            if(recievedData > 0)
            {
                cout << "message recieved" << endl;
                sock.recvFrom((char*) message, MSGSIZE); //recieve the information
                if(message[0] == i) //only if its the correct one
                {
                    cout << "ack sent back to client" << endl;
                    sock.ackTo((char *) &i, sizeof(i)); //if data has been receievd then I need to send it acknoledge it 
                    break;                
                }
            }            
         }
    }
}

/**
 *sends message[] and receiving an acknowledgment from a server max (=20,000) times using the sock object.
 * As described above, the client can continuously send a new message[] and increasing the sequence number as long as the number of in-transit messages (i.e., # of unacknowledged messages) is less than "windowSize." 
 * That number should be decremented every time the client receives an acknowledgment. If the number of unacknowledged messages reaches "windowSize," the client should start a Timer. If a timeout occurs (i.e., no response after 1500 usec), 
 * it must resend the message with the minimum sequence number among those which have not yet been acknowledged. 
 * The function must count the number of messages (not bytes) re-transmitted and return it to the main function as its return value. 
 * @param socket the udp socket send data from
 * @param int the maximum number of time (messages) to send
 * @param message the message buffer
 * @param windowsize the windowSize to the send the data
 * @return number of retransmission
 **/
int clientSlidingWindow( UdpSocket &sock, const int max, int message[], int windowSize )
{
    cout << "beginning clientSlidingWindow" << endl;
    int resubmissions = 0;
    int unacknowledged = 0;
    int acknowledgements = 0;
    for(int i = 0; i < max; i++)
    {
        cerr << "unacknowledged: " << unacknowledged << " windowsize: " << windowSize << endl;
        if(unacknowledged < windowSize)//if the unacknowledged messages is less than windowsize 
        {
            message[0] = i; //insert the message into message[0]
            sock.sendTo((char*) message, MSGSIZE); //send the message to the server
            unacknowledged++;
            cerr << " i " << i << " unacknolwedged: " << unacknowledged << endl;
        }

        if(unacknowledged == windowSize) //has to be another if statment here otherwise breaks
        {
            Timer timer;
            timer.start();
            while(true)
            {
                if(sock.pollRecvFrom() > 0)
                {
                    sock.recvFrom((char *)message, MSGSIZE);
                    if(message[0] < 0) {//The server is done
                        i = max; 
                        break;
                    }
                    cerr << "recieved data " << message[0] << " acknowledgements " << acknowledgements << endl;
                    if(message[0] == acknowledgements)
                    {
                        cout << "correct message couting as acknolegement" << endl;
                        acknowledgements++; //increase the amount acknowledged
                        unacknowledged--; //take away from the for loop total     
                        break; //response is true and then cut the while loop
                    }
                }
                if(timer.lap() > TIMEOUT && unacknowledged == windowSize) //If we go over the 1500 Timeout 
                {
                    cerr << "Timed Out resending message" << endl;
                    resubmissions = resubmissions + (i + windowSize - acknowledgements); //add to resubmissions count
                    i = acknowledgements; //resetting back to the last correctly submitted ack 
                    unacknowledged = 0; //go back to last valid ack
                    cerr << "after acknolegements i: " << i << endl;
                    break;
                }       
            }
        }
    }
    cout << "finishing" << endl;
    return resubmissions;
}

/**
 * receives message[] and sends an acknowledgment to the client max (=20,000) times using the sock object. 
 * Every time the server receives a new message[], it must save the message's sequence number in an array and return a cumulative acknowledgment, 
 * i.e.,* the last received message in order.
 * @param socket the udp socket retrieve data from
 * @param int the maximum number of time (messages) to retrieve
 * @param message the message buffer
 * @param windowsize the windowSize to the retrieve the data
 **/
void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], int windowSize )
{
    cout << "inside Server EarlyRetrans" << endl;
    int cumAck[max];
    int count = 0;
    while(count != max)
    {
        if(sock.pollRecvFrom() > 0)
        {
            //cout << "message recieved" << endl;
            sock.recvFrom((char*) message, MSGSIZE)  ; //recieve the information
            cout << "message: " << message[0] << " count: "  << count << endl;
            if(message[0] == count){
                cout << "acknolegment sent" << endl; //never gets in here
                sock.ackTo((char *) &count, sizeof(count)); //if data has been receievd then I need to send it acknoledge it 
                cumAck[count] = message[0];
                count++;
            }
            else
            {
                sock.ackTo((char *) &count, sizeof(count)); //if message[0] != count 2 != 3
                count++; //not sure
            }
        }   
    }
    count = -1;
    sock.ackTo((char* ) &count, sizeof(count));
}