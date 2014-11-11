#include <iostream>
#include "IrcBot.h"

using namespace std;


int main()
{
    char channel[50];
    cout << "Channel : ";
    cin >> channel;
	IrcBot bot = IrcBot("PASS oauth:w72sl1c3tt25nljt880939fqc8wio7\r\n","NICK Thedarkbot_\r\n", channel);
	bot.start();

    return 0;

}
