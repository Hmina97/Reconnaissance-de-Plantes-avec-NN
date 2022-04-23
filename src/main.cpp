#include <string>
#include "../include/Output.h"

int main()
{
	std::string trainingPath = "trainset_3665";
	std::string testingPath = "testset_700";
	output* cnn = new output(trainingPath, testingPath);
	// Pour une grande precision, on doit definir plus d'epoch
	// Le deuxieme parametre est le taux d'apprentissage
	cnn->Training_data(1, 0.003);
	cnn->Testing_data();
	delete cnn;
	return 0;
}
