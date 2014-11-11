/*
 * IrcBot.h
 *
 *  Cree le: 01 Nov 2014
 *      Auteur: thedarkfly
 *      Sur base de: Tyler Allen
 */

#ifndef IRCBOT_H_
#define IRCBOT_H_

#include <string>
#include <vector>

class IrcBot
{
public:
	IrcBot(char * _nick, char * _pass, char * _channel);
	virtual ~IrcBot();

	bool setup;

	void start();

	bool charSearch(char *toSearch, char *searchFor);
	bool stringSearch(std::string toSearch, std::string searchFor);
	std::string transformToNeutral(std::string toTransform);
	std::string toLower(std::string toTransform);
	std::string removePunct(std::string toTransform);
	std::string getMessage(std::string toFilter);
    std::string getPseudo(std::string toFilter);
    char* convert(std::string toConvert);
    std::string getCommand(std::string toFilter);
    void sendMessage(std::string msg);
    std::string getArgument(std::string toFilter);

    std::vector<std::string> readCommandsFile();
    std::vector<std::string> readTextsFile();
    std::string addCommand(std::string com, std::string txt);
    std::string delCommand(std::string com);
    void saveCommands();
    void commandCalled(std::string com);
    std::string editCommand(std::string com, std::string txt);

    int compteurMaj(std::string msg); //TODO
    int compteurMaxChar(std::string msg); //TODO
    int compteurEmote(std::string msg); //TODO

    void addOp(std::string msg);
    void delOp(std::string msg);
    bool isOp(std::string pseudo);

    void timeout(std::string pseudo, int time);

    int occurrences(std::string toSearch, std::string searchFor);
    int arrOccurrences(std::string toSearch, std::vector<std::string> arr);
    int capsNbr(std::string toSearch);
    int emoteNbr(std::string toSearch);

private:
	char *port;
	int s; //descripteur de socket

	char *nick;
	char *pass;
	char *channel;

	bool isConnected(char *buf);

	char * timeNow();

	bool sendData(char *msg);

	void sendPong(char *buf);

	void msgHandel(char *buf);

	bool livePrevu;
	bool activate;
	bool nazi;
	std::vector<std::string> commands[2];
	std::vector<std::string> ops;
	int punishmentTable[7];

};

#endif /* IRCBOT_H_ */
