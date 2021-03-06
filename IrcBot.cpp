/*
 * IrcBot.cpp
 *
 *  Cree le: 01 Nov 2014
 *      Auteur: thedarkfly
 *      Sur base de: Tyler Allen
 */

#include "IrcBot.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <string>
#include <algorithm>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

#define MAXDATASIZE 1000

IrcBot::IrcBot(char * _nick, char * _pass, char * _channel)
{
	nick = _nick;
	pass = _pass;
	channel = _channel;

    commands[0] = readCommandsFile();
    commands[1] = readTextsFile();

    emotes = readEmotesFile();
    vector<int> limits = readLimitsFile();
    limitePave = limits[0];
    limiteCaps = limits[1];
    limiteEmotes = limits[2];
    limiteLettres = limits[3];

    punishmentTable[0] = 5;
    punishmentTable[1] = 60;
    punishmentTable[2] = 300;
    punishmentTable[3] = 600;
    punishmentTable[4] = 1800;
    punishmentTable[5] = 3600;
    punishmentTable[6] = 28800;

    livePrevu = false;
    activate = true;
    nazi = false;
    muted = false;
    finished = false;
    interaction = true;

}

IrcBot::~IrcBot()
{
	close (s);
}

void IrcBot::start()
{
    srand(time(0));

	struct addrinfo hints, *servinfo;

	//Setup sans erreurs
	setup = true;

	port = "6667";

	//Ensure that servinfo is clear
	//Verifie si servinfo est valide
	memset(&hints, 0, sizeof hints); // s'assurer que la structure est vide

	//setup hints
	hints.ai_family = AF_UNSPEC; // IPv4 ou IPv6, osef
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

	//Setup les structs si l'erreur demande pourquoi
	int res;
	if ((res = getaddrinfo("irc.twitch.tv",port,&hints,&servinfo)) != 0)
	{
		setup = false;
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(res));
	}

	//Setup la socket
	if ((s = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol)) == -1)
	{
		perror("client: socket");
	}

	//Connection
	if (connect(s,servinfo->ai_addr, servinfo->ai_addrlen) == -1)
	{
		close (s);
		perror("Client Connect");
	}

	//On en a plus besoin
	freeaddrinfo(servinfo);

	//Pour recuperer les donnees
	int numbytes;
	char buf[MAXDATASIZE];

	int counter = 0;
	while (1)
	{
		//declaration
		counter++;

		switch (counter) {
			case 1:
					//d'abord se connecter
					sendData(nick);
					sendData(pass);
				break;
			case 2: {
                        //puis choisir son channel
                        string strChannel(channel);
                        char* data = convert("JOIN #" + strChannel + "\r\n");
                        sendData(data);
                    }
			default:
                break;
		}

		//Recevoir et imprimer les donnees
		numbytes = recv(s,buf,MAXDATASIZE-1,0);
		buf[numbytes]='\0';

		cout << buf;
		//buf = donnees sous forme de tableau de char

		//On analyse le buf pour y reagir
		msgHandel(buf);


		//Si un Ping est recu
		if (charSearch(buf,"PING"))
		{
			sendPong(buf);
		}

		//break si la connectoin est fermee
		if (numbytes==0)
		{
			cout << "----------------------CONNECTION CLOSED---------------------------"<< endl;
			cout << timeNow() << endl;

			break;
		}
	}
}


bool IrcBot::charSearch(char *toSearch, char *searchFor)
{//on l'utilise plus, voir stringSearch
	int len = strlen(toSearch);
	int forLen = strlen(searchFor); // La longueur du champ de recherche

	//Recherche dans chaque caractere de toSearch
	for (int i = 0; i < len;i++)
	{
		//Si le char actif est egal au premier element qu'on recherche, alors chercher dans toSearch
		if (searchFor[0] == toSearch[i])
		{
			bool found = true;
			//on cherche le tableau de char pour un champ de recherche
			for (int x = 1; x < forLen; x++)
			{
				if (toSearch[i+x]!=searchFor[x])
				{
					found = false;
				}
			}

			//si on a trouve ce que l'on cherche, return true;
			if (found == true)
				return true;
		}
	}

	return 0;
}

bool IrcBot::isConnected(char *buf)
{//retourne true si "/MOTD" est trouve dans l'entree
	//Si on trouve /MOTD alors on peut rejoindre le channel
	if (charSearch(buf,"/MOTD") == true)
		return true;
	else
		return false;
}

char * IrcBot::timeNow()
{//retourne la date et l'heure
	time_t rawtime;
	struct tm * timeinfo;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	return asctime (timeinfo);
}


bool IrcBot::sendData(char *msg)
{//Envoie des donnees
	int len = strlen(msg);
	int bytes_sent = send(s,msg,len,0);
	cout << "Data sent : " << msg;

	if (bytes_sent == 0)
		return false;
	else
		return true;
}

void IrcBot::sendPong(char *buf)
{
	//Chope l'adresse de reponse
	//boucle dans buf et on trouve la location du PING
	//Cherche dans chaque caractere de toSearch

	char * toSearch = "PING ";

	for (int i = 0; i < strlen(buf);i++)
		{
		//Si le char actif est egal au premier element qu'on recherche, alors chercher dans toSearch
			if (buf[i] == toSearch[0])
			{
				bool found = true;
			//on cherche le tableau de char pour un champ de recherche
				for (int x = 1; x < 4; x++)
				{
					if (buf[i+x]!=toSearch[x])
					{
						found = false;
					}
				}

				//si on a trouve, on return true;
				if (found == true)
				{
					int counter = 0;
					//On compte les caracteres
					for (int x = (i+strlen(toSearch)); x < strlen(buf) ;x++)
					{
                        if(isspace(buf[x]) && buf[x] != ' ')
                            break; //si on rencontre un caractère spécial, on arrete

						counter++;
					}
					counter += 3; //pour les caractères "invisibles" /r, /n et /0

					//On cree un nouveau tableau de caracteres
					char returnHost[counter + 5];
					returnHost[0]='P';
					returnHost[1]='O';
					returnHost[2]='N';
					returnHost[3]='G';
					returnHost[4]=' ';


					counter = 0;
					//on definit la donnee hostname
					for (int x = (i+strlen(toSearch)); x < strlen(buf) ;x++)
					{
                        if(isspace(buf[x]) && buf[x] != ' ')
                            break;// idem, caractere special = arret

						returnHost[counter+5]=buf[x];
						counter++;
					}
					counter += 5; // pour "PONG " (5 caracteres)
					returnHost[counter] = '\r';
					returnHost[counter + 1] = '\n';
					returnHost[counter + 2] = '\0'; // les trois caracteres speciaux

					//on envoie pong
					if (sendData(returnHost))
					{
						cout << timeNow() <<"  Ping Pong" << endl;
					}


					return;
				}
			}
		}

}

bool IrcBot::stringSearch(string toSearch, string searchFor)
{//Rechercher un string dans un autre
    return toSearch.find(searchFor) != string::npos;
}

string IrcBot::transformToNeutral(string toTransform)
{//Transforme en texte full minuscules sans ponctuation
    return removePunct(toLower(toTransform));
}

string IrcBot::removePunct(string toTransform)
{//Enleve toute ponctuation
    for(int i = 0; i < toTransform.size(); i++)
    {
        if(ispunct(toTransform[i]))
            toTransform.erase(i, 1);
    }

    return toTransform;
}

string IrcBot::toLower(string toTransform)
{//Met toutes les majuscules en minuscules
    transform(toTransform.begin(), toTransform.end(), toTransform.begin(), ::tolower);
    return toTransform;
}

string IrcBot::getMessage(string toFilter)
{//Retourne le message qu'un utilisateur a envoye via le chat
    int index = toFilter.find(" :");

    return toFilter.substr(index+2, toFilter.size() - index - 4);
}

string IrcBot::getPseudo(string toFilter)
{//Retourne le pseudo de l'utilisateur qui a envoye le message
    int index1 = toFilter.find(":");
    int index2 = toFilter.find("!");

    return toFilter.substr(index1+1, index2-1);
}

string IrcBot::getCommand(string toFilter)
{//Retourne la commande envoyee
    //Attention, a utiliser qu'en cas de commande
    int index = toFilter.find(" ");

    if(index != string::npos)
        return toFilter.substr(1, index-1);
    else
        return toFilter.substr(1);
}

string IrcBot::getArgument(string toFilter)
{//Retourne l'argument d'une commande
    //Attention, n'utiliser qu'en cas de commande + argument
    int index = toFilter.find(" ");

    return toFilter.substr(index+1);
}

char* IrcBot::convert(string toConvert)
{//Convertit un string en tableau de caractere pour l'envoi de donnees
    char* arr = new char[toConvert.size() + 1];
    arr[toConvert.size()] = 0;
    memcpy(arr, toConvert.c_str(), toConvert.size());
    return arr;
}

void IrcBot::sendMessage(string msg)
{//Envoie un message sur le chat
    if(!muted)
    {
        string strChannel(channel);
        char* data = convert("PRIVMSG #" + strChannel + " :" + msg + "\r\n");
        sendData(data);
    }
}

vector<int> IrcBot::readLimitsFile()
{
    string line;
    int i = 0;
    ifstream file("limits[pave-majs-emote-lettres].dat");
    vector<int> limits(4);

    if(file.is_open())
    {
        while(getline(file, line))
        {
            switch(i)
            {
                case 0:
                    istringstream(line) >> limits[0];
                    break;
                case 1:
                    istringstream(line) >> limits[1];
                    break;
                case 2:
                    istringstream(line) >> limits[2];
                    break;
                case 3:
                    istringstream(line) >> limits[3];
                    break;
                default:
                    break;
            }
            i++;
        }
        file.close();
    }
    return limits;
}

vector<string> IrcBot::readEmotesFile()
{
    vector<string> readEmotes;

    string line;
    ifstream file("emotes.dat");

    if(file.is_open())
    {
        while(getline(file, line))
        {
            readEmotes.push_back(line);
        }
        file.close();
    }
    return readEmotes;
}

vector<string> IrcBot::readCommandsFile()
{
    vector<string> readCommands;
    int alternator = 0;

    string line;
    ifstream file("commands.dat");

    if(file.is_open())
    {
        while(getline(file, line))
        {
            if(alternator == 0)
            {
                readCommands.push_back(line);
                alternator = 1;
            }
            else if(alternator == 1)
            {
                alternator = 0;
            }
        }
        file.close();
    }
    return readCommands;
}

vector<string> IrcBot::readTextsFile()
{
    vector<string> readTexts;
    int alternator = 1;

    string line;
    ifstream file("commands.dat");

    if(file.is_open())
    {
        while(getline(file, line))
        {
            if(alternator == 0)
            {
                readTexts.push_back(line);
                alternator = 1;
            }
            else if(alternator == 1)
            {
                alternator = 0;
            }
        }
        file.close();
    }
    return readTexts;
}

string IrcBot::addCommand(string com, string txt)
{
    for(vector<string>::iterator it = commands[0].begin(); it != commands[0].end(); ++it)
    {
        if((*it).compare(com) == 0)
        {
            return "la commande ?" + com + " existe déjà. Dommage !";
        }
    }

    commands[0].push_back(com);
    commands[1].push_back(txt);

    saveCommands();

    return "la commande ?" + com + " a bien été ajoutée. Félicitations. Wow. Such skillz.";
}

string IrcBot::delCommand(string com)
{
    int index = 0;
    bool done = false;

    for(vector<string>::iterator it = commands[0].begin(); it != commands[0].end(); ++it)
    {
        if ((*it).compare(com) == 0)
        {
            done = true;
            string txt = commands[1][index];
            for(vector<string>::iterator id = commands[1].begin(); id != commands[1].end(); ++id)
            {
                if((*id).compare(txt) == 0)
                {
                    commands[1].erase(id);
                    break;
                }
            }
            commands[0].erase(it);
            break;
        }
        else index++;
    }
    saveCommands();
    if(done)
        return "la commande ?" + com + " a bien été supprimée. Pour toujours et à jamais. Amen.";
    else if(!done)
        return "la commande ?" + com + " n'existe pas, bénêt.";
}

string IrcBot::editCommand(string com, string txt)
{
    int index = 0;
    bool done = false;

    for(vector<string>::iterator it = commands[0].begin(); it != commands[0].end(); ++it)
    {
        if((*it).compare(com) == 0)
        {
            done = true;
            commands[1][index] = txt;
        }
        else index++;
    }
    saveCommands();
    if(done)
        return "la commande ?" + com + " a bien été modifiée. Félicitations. Quelle maîtrise.";
    else if(!done)
        return "la commande ?" + com + " n'existe pas, bouffon.";
}

void IrcBot::saveLimits()
{
    ofstream file("limits[pave-majs-emote-lettres].dat");

    if(file.is_open())
    {
        ostringstream p;
        ostringstream c;
        ostringstream e;
        ostringstream l;

        p << limitePave << "\n";
        c << limiteCaps << "\n";
        e << limiteEmotes << "\n";
        l << limiteLettres << "\n";

        file << p.str();
        file << c.str();
        file << e.str();
        file << l.str();

        file.close();
    }
}

void IrcBot::saveEmotes()
{
    ofstream file("emotes.dat");
    int i = 0;

    if(file.is_open())
    {
        for(vector<string>::iterator it = emotes.begin(); it != emotes.end(); it++)
        {
            file << emotes[i] + "\n";
            i++;
        }
        file.close();
    }
}

void IrcBot::saveCommands()
{
    ofstream file ("commands.dat");
    int i = 0;

    if(file.is_open())
    {
        for(vector<string>::iterator it = commands[0].begin(); it != commands[0].end(); ++it)
        {
            file << commands[0][i] + "\n";
            file << commands[1][i] + "\n";
            i++;
        }
        file.close();
    }
}

void IrcBot::commandCalled(string com)
{
    int index = 0;

    for(vector<string>::iterator it = commands[0].begin(); it != commands[0].end(); ++it)
    {
        if ((*it).compare(com) == 0)
        {
            sendMessage(commands[1][index]);
        }
        else index++;
    }
}

void IrcBot::msgHandel(char * buf)
{//Reagit aux evenements sur le chat
    string str(buf);

    if(activate)
    {
        //REACTION TEXTUELLE
        if(stringSearch(str, "PRIVMSG #"))
        {
            string strChannel(channel);
            string message = getMessage(str);
            string neutralMessage = transformToNeutral(message);
            string pseudo = getPseudo(str);

            if(interaction)
            {
                //Y A-T-IL UN LIVE DE PREVU ?
                if((stringSearch(neutralMessage, "live")
                        || stringSearch(neutralMessage, "stream"))
                    && (stringSearch(message, "?"))
                    && (stringSearch(neutralMessage, "ce soir")
                        || stringSearch(neutralMessage, "aujourdhui")
                        || stringSearch(neutralMessage, "prévu")
                        || stringSearch(neutralMessage, "quand")
                        || stringSearch(neutralMessage, "kan")))
                {
                    if(livePrevu)
                        sendMessage("Il y a un live prévu, " + pseudo + " ! Mais on ne sait pas quand.");
                    else
                        sendMessage("Aucun live de prévu aujourd'hui, désolé " + pseudo + ". Vérifie régulièrement les réseaux sociaux.");
                }
                else if((stringSearch(neutralMessage, "live")
                        || stringSearch(neutralMessage, "stream"))
                    && (stringSearch(message, "?"))
                    && (stringSearch(message, "fini")
                        || stringSearch(message, "termine")))
                {
                    if(finished)
                        sendMessage("Le live est terminé. La rediffusion est disponible ici : http://wankil.reddit.com");
                }

                //REACTIONS BANALES
                else if(stringSearch(neutralMessage, "too many cooks"))
                {
                    sendMessage("TOO MANY COOKS !");
                }

                else if(stringSearch(neutralMessage,"et toi darkbot"))
                {
                    int random = rand() % 6;

                    switch (random)
                    {
                        case 0:
                            sendMessage("Posé, merci.");
                            break;
                        case 1:
                            sendMessage("Toujours en vie, apparemment.");
                            break;
                        case 2:
                            sendMessage("J'en sais rien j'suis un bot.");
                            break;
                        case 3:
                            sendMessage("Mal, ma copine m'a lâché. :(");
                            break;
                        case 4:
                            sendMessage("Je sais pas, devine.");
                            break;
                        case 5:
                            sendMessage("Demande à Darkfly, c'est pas à moi de décider.");
                            break;
                        default:
                            break;
                    }
                }

                else if(stringSearch(neutralMessage,"autant pour moi"))
                {
                    sendMessage("Au temps*");
                }

                else if(stringSearch(neutralMessage, "nightbot tried") && pseudo.compare("nightbot") == 0)
                {
                    sendMessage("Tg Nightbot, tu sers à rien.");
                }

                else if((stringSearch(neutralMessage,"salut")
                    || stringSearch(neutralMessage,"bonjour")
                    || stringSearch(neutralMessage,"bonsoir")
                    || stringSearch(neutralMessage,"yo")
                    || stringSearch(neutralMessage,"yop")
                    || stringSearch(neutralMessage,"coucou")
                    || stringSearch(neutralMessage,"cc")
                    || stringSearch(neutralMessage,"slt")
                    || stringSearch(neutralMessage,"hey"))
                    && stringSearch(neutralMessage,"darkbot"))
                {
                    sendMessage("Salut " + pseudo + ", comment ça va ?");
                }

                else if((stringSearch(neutralMessage,"au revoir")
                    || stringSearch(neutralMessage,"bonne nuit")
                    || stringSearch(neutralMessage,"bonne journee")
                    || stringSearch(neutralMessage,"a la prochaine")
                    || stringSearch(neutralMessage,"bye")
                    || stringSearch(neutralMessage,"ciao")
                    || stringSearch(neutralMessage,"a plus")
                    || stringSearch(neutralMessage,"a toute")
                    || stringSearch(neutralMessage,"a tout a lheure")
                    || stringSearch(neutralMessage,"a tantot")
                    || stringSearch(neutralMessage,"a bientot"))
                    && stringSearch(neutralMessage,"darkbot"))
                {
                    sendMessage("À très bientôt " + pseudo + ", j'espère. o/");
                }
            }

            //COMMANDES
            if(stringSearch(message,"?") && (isOp(pseudo) || pseudo.compare("thedarkfly") == 0))
            {
                string command(getCommand(message));

                commandCalled(command);

                if(command.compare("addcom") == 0)
                {
                    string toAdd(getArgument(message));
                    string commandToAdd = getCommand(toAdd);

                    if(toAdd.length() - commandToAdd.length() <= 2)
                        sendMessage("Connard de " + pseudo + ", il manque quelque chose, là. Crétin.");
                    else
                    {
                        string textToAdd = toAdd.substr(2 + commandToAdd.length());
                        sendMessage(pseudo + ", " + addCommand(commandToAdd, textToAdd));
                    }
                }
                else if(command.compare("delcom") == 0)
                {
                    string toDel(getArgument(message));
                    string commandToDel = getCommand(toDel);

                    sendMessage(pseudo + ", " + delCommand(commandToDel));
                }
                else if(command.compare("editcom") == 0)
                {
                    string toEdit(getArgument(message));
                    string commandToEdit = getCommand(toEdit);

                    if(toEdit.length() - commandToEdit.length() <= 2)
                        sendMessage("Connard de " + pseudo + ", il manque quelque chose, là. Crétin.");
                    else
                    {
                        string textToEdit = toEdit.substr(2 + commandToEdit.length());
                        sendMessage(pseudo + ", " + editCommand(commandToEdit, textToEdit));
                    }
                }
                else if(command.compare("mute")== 0)
                {
                    sendMessage("Mode muet activé.");
                    muted = true;
                }
                else if(command.compare("unmute") == 0)
                {
                    muted = false;
                    sendMessage("Mode muet désactivé.");
                }
                else if(command.compare("desactive") == 0)
                {
                    sendMessage("Désactivation... À la prochaine o/");
                    activate = false;
                }
                else if(command.compare("prevu") == 0)
                {
                    string argument(getArgument(message));
                    if(argument.compare("oui") == 0)
                    {
                        livePrevu = true;
                        sendMessage("Message reçu : un live est prévu ce soir. Youpie !");
                    }
                    else if(argument.compare("non") == 0)
                    {
                        livePrevu = false;
                        sendMessage("Message reçu : aucun live de prévu ce soir. Flûte de zut.");
                    }
                }
                else if(command.compare("fini") == 0)
                {
                    string argument(getArgument(message));
                    if(argument.compare("oui") == 0)
                    {
                        finished = true;
                        sendMessage("Message reçu : le live est terminé.");
                    }
                    else if(argument.compare("non") == 0)
                    {
                        finished = false;
                    }
                }
                else if(command.compare("nazi") == 0)
                {
                    string argument(getArgument(message));
                    if(argument.compare("on") == 0)
                    {
                        nazi = true;
                        sendMessage("Mode nazi activé. Ça va faire mal. MrDestructoid");
                    }
                    else if(argument.compare("off") == 0)
                    {
                        nazi = false;
                        sendMessage("Mode nazi désactivé. On s'est bien marré quand même !");
                    }
                }
                else if(command.compare("interaction") == 0)
                {
                    string argument(getArgument(message));
                    if(argument.compare("on") == 0)
                    {
                        interaction = true;
                        sendMessage("Réponses automatiques activées. Flemmards de modos, va.");
                    }
                    else if(argument.compare("off") == 0)
                    {
                        interaction = false;
                        sendMessage("Réponses automatiques désactivées. Enfin un peu de glande.");
                    }
                }
                else if(command.compare("addemote") == 0)
                {
                    string argument(getArgument(message));

                    emotes.push_back(argument);
                    saveEmotes();
                    sendMessage("Émoticône ajoutée avec succès : " + argument + ".");
                }
                else if(command.compare("setcapslimit") == 0)
                {
                    string argument(getArgument(message));

                    istringstream(argument) >> limiteCaps;

                    saveLimits();
                    sendMessage("Limite du nombre de majuscules fixée à " + argument + ".");
                }
                else if(command.compare("setcharlimit") == 0)
                {
                    string argument(getArgument(message));

                    istringstream(argument) >> limitePave;

                    saveLimits();
                    sendMessage("Limite du nombre de caractères fixée à " + argument + ".");

                }
                else if(command.compare("setemotelimit") == 0)
                {
                    string argument(getArgument(message));

                    istringstream(argument) >> limiteEmotes;

                    saveLimits();
                    sendMessage("Limite du nombre d'émoticônes fixée à " + argument + ".");

                }
                else if(command.compare("setletterspamlimit") == 0)
                {
                    string argument(getArgument(message));

                    istringstream(argument) >> limiteLettres;

                    saveLimits();
                    sendMessage("Limite du nombre de lettres identiques fixée à " + argument + ".");

                }
                else if(command.compare("insulte") == 0)
                {
                    string argument(getArgument(message));

                    int random = rand() % 6;

                    switch (random)
                    {
                        case 0:
                            sendMessage("Pd de " + argument + ".");
                            break;
                        case 1:
                            sendMessage(argument + ", t'es un connard.");
                            break;
                        case 2:
                            sendMessage("Je t'emmerde, " + argument + ".");
                            break;
                        case 3:
                            sendMessage("Va bien t'faire f*tre, " + argument + ".");
                            break;
                        case 4:
                            sendMessage("Je te méprise, " + argument + ". Cordialement.");
                            break;
                        case 5:
                            sendMessage(argument + ", tu sens pas bon des pieds.");
                            break;
                        default:
                            break;
                    }
                }

                //COMMANDES DE MODERATION
                if(command.compare("purge") == 0)
                {
                    string argument(getArgument(message));
                    timeout(argument, 1);
                    sendMessage(argument + " purgé avec succès.");
                }
                else if(command.compare("timeout") == 0)
                {
                    string argument(getArgument(message));
                    timeout(argument, 60);
                    sendMessage(argument + " banni pendant dix minutes.");
                }
                else if(command.compare("ban") == 0)
                {
                    string argument(getArgument(message));
                    timeout(argument, -1);
                    sendMessage(argument + " banni définitivement.");
                }
                else if(command.compare("unban") == 0)
                {
                    string argument(getArgument(message));
                    timeout(argument, 0);
                    sendMessage(argument + " a retrouvé le droit à la parole.");
                }
                else if(command.compare("ciao") == 0)
                {
                    string argument(getArgument(message));
                    timeout(argument, 28800);
                    sendMessage("À la prochaine, " + argument + " !");
                }
            }


            //MODERATION AUTOMATIQUE
            if(nazi && capsNbr(message) > limiteCaps)// && !isOp(pseudo) && pseudo.compare("thedarkfly") != 0)
            {
                timeout(pseudo, 1);
                sendMessage(pseudo + ", mollo sur les majuscules, merci.");
            }

            if(nazi && emoteNbr(message) > limiteEmotes)// && !isOp(pseudo) && pseudo.compare("thedarkfly") != 0)
            {
                timeout(pseudo, 1);
                sendMessage(pseudo + ", doucement avec les émoticônes, merci.");
            }

            if(nazi && message.length() > limitePave)// && !isOp(pseudo) && pseudo.compare("thedarkfly") != 0)
            {
                timeout(pseudo, 1);
                sendMessage(pseudo + ", pas de pavé, merci.");
            }

            if(nazi && sameCharNbr(message) > limiteLettres)// && !isOp(pseudo) && pseudo.compare("thedarkfly") != 0)
            {
                timeout(pseudo, 1);
                sendMessage(pseudo + ", pas de spam, merci.");
            }

        }

        //DETECTION DES MODOS
        else if(stringSearch(str, "MODE #"))
        {
            if(stringSearch(str, "+o"))
                addOp(str);
            else if(stringSearch(str, "-o"))
                delOp(str);
        }
    }
    else
    {
        string message = getMessage(str);
        string pseudo = getPseudo(str);
        if(stringSearch(message,"?") && (isOp(pseudo) || pseudo.compare("thedarkfly") == 0))
        {
            string command(getCommand(message));
            if(command.compare("active") == 0)
            {
                activate = true;
                sendMessage("Activation... Salut tout le monde ! o/");
            }
        }
    }
}

void IrcBot::addOp(string msg)
{
    if(stringSearch(msg, "+o"))
    {
        int index1 = msg.find("+o ");
        int index2 = 0;

        for(string::iterator it = msg.begin() + index1 + 3; it != msg.end(); it++)
        {
            if(isspace(*it))
                break;

            index2++;
        }

        ops.push_back(msg.substr(index1 + 3, index2));
        addOp(msg.substr(index1 + 3));
    }
}

void IrcBot::delOp(string msg)
{
    int index = msg.find("-o ");
    string op = msg.substr(index+3, msg.size() - index - 4);

    for(vector<string>::iterator it = ops.begin(); it != ops.end(); ++it)
    {
        if((*it).compare(op) == 0)
        {
            ops.erase(it);
            break;
        }
    }
}

bool IrcBot::isOp(string pseudo)
{
    for(vector<string>::iterator it = ops.begin(); it != ops.end(); ++it)
    {
        if((*it).compare(pseudo) == 0)
        {
            return true;
        }
    }
    return false;
}

void IrcBot::timeout(string pseudo, int time)
{//timeout un utilisateur.
    //TEMPS NEGATIF : Ban définitif
    //TEMPS NUL : Unban
    //TEMPS POSITIF : Timeout avec duree specifiée
    if(time < 0)
        sendMessage("/ban " + pseudo);
    else if(time == 0)
        sendMessage("/unban " + pseudo);
    else
    {
        string strTime;
        ostringstream convert;
        convert << time;
        strTime = convert.str();

        sendMessage("/timeout " + pseudo + " " + strTime);
    }
}

int IrcBot::occurrences(string toSearch, string searchFor)
{
    int tsLen = toSearch.length();
    int sfLen = searchFor.length();
    int pos = toSearch.find(searchFor);

    if(pos == string::npos)
        return 0;
    else
        return 1 + occurrences(toSearch.substr(pos + sfLen), searchFor);
}

int IrcBot::arrOccurrences(string toSearch, vector<string> arr)
{
    int accumulator = 0;

    for(vector<string>::iterator it = arr.begin(); it != arr.end(); it++)
    {
        accumulator += occurrences(toSearch, *it);
    }

    return accumulator;
}

int IrcBot::capsNbr(string toSearch)
{
    string arr[] = {"A", "B", "C", "D", "E", "F", "G", "H",
        "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
        "U", "V", "W", "V", "X", "Y", "Z"};

    vector<string> caps (arr, arr + sizeof(arr) / sizeof(string));

    return arrOccurrences(toSearch, caps);
}

int IrcBot::emoteNbr(string toSearch)
{
    return arrOccurrences(toSearch, emotes);
}

int IrcBot::sameCharNbr(string toSearch)
{
    int acc1 = 0;
    int acc2 = 1;
    int n = 0;

    for(string::iterator it = toSearch.begin(); it != toSearch.end() - 1; it++)
    {
        if(toSearch[acc1] == toSearch[acc2])
        {
            n++;
        }
        else
            n = 0;
        acc1++;
        acc2++;
    }

    return n;
}
