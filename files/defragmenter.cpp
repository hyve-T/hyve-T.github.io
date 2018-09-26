#include "defragmenter.h"
#include "DefragRunner.h"
#include "mynew.h"

using namespace std;

extern int currentRAM;

Defragmenter::Defragmenter(DiskDrive *diskDrive) {
    unsigned* startPos = new unsigned[diskDrive->getNumFiles()];
    pair<unsigned short, unsigned short> *parallelDir = new pair<unsigned short, unsigned short>[diskDrive->getCapacity()]; //[i][0] is short filenum, [i][1] is short bloknum (within file)
    bool* correct = new bool[diskDrive->getCapacity()];
    
    for (int i = 0; i < diskDrive->getCapacity(); i++)
        correct[i] = false;
    
    
    calcStartPos(diskDrive, startPos); //array will be used to calculate all block desired positions, and set firstBlockIDs in Directory
    
    createDir(diskDrive, parallelDir, correct, startPos);//finds and records the positions of each file part in the disk
    
    defrag(diskDrive, startPos, parallelDir, correct); //main loop  
    
    delete [] startPos;
    
    delete [] parallelDir;

} // Defragmenter()

void Defragmenter::calcStartPos(DiskDrive* diskDrive, unsigned* startPos) {
    startPos[0] = 2;
    diskDrive->directory[0].setFirstBlockID(startPos[0]) ;


    for (int i = 1; i < diskDrive->getNumFiles(); i++) {
        startPos[i] = startPos[i - 1] + diskDrive->directory[i - 1].getSize();
        diskDrive->directory[i].setFirstBlockID(startPos[i]);
    }
}

void Defragmenter::createDir(DiskDrive *diskDrive, pair<unsigned short, unsigned short> *parallelDir, bool* correct, unsigned* startPos) {

	int pos;
    DiskBlock* tempBlockPtr = NULL;
    

    for (int i = 0; i < diskDrive->getNumFiles(); i++) {    //for each file

        pos = diskDrive->directory[i].getFirstBlockID();

        for (unsigned short j = 0; j < diskDrive->directory[i].getSize(); j++) {    //for each block of the file
            //traverse and mark pos (as you're following nextBlockID) with fileNum and j
            parallelDir[pos].first = i;
            parallelDir[pos].second = j;
            
            if(pos == desiredPos(parallelDir[pos], startPos))
                correct[pos] = true;
            
            tempBlockPtr = diskDrive->readDiskBlock(pos);
            pos = tempBlockPtr->getNext(); //go to next in trail 
            delete tempBlockPtr;

        } //set values for all diskBlocks of ith file        
    } // does this for every file

}

void Defragmenter::defrag(DiskDrive *diskDrive, unsigned* startPos, pair<unsigned short, unsigned short> *parallelDir, bool* correct) {

    //bool* correct = new bool[diskDrive->getCapacity()];
    DiskBlock* tempBlockPtr = NULL;
    
    for (unsigned i = 2; i < (unsigned)diskDrive->getCapacity(); i++) { //checks every spot in file
        //if bool correct true or value is  0 0 0, skip
        //else if pos incorrect, reorder(i) and set bool true
        if (correct[i] || !diskDrive->FAT[i]) //if we've been over it or if it's empty
            continue;

        if ((unsigned)desiredPos(parallelDir[i], startPos) != i)
            reorder(diskDrive, parallelDir, i, startPos, correct); //will rearrange original & parallel, update FAT, update bool correct
        else {
            tempBlockPtr = diskDrive->readDiskBlock(i);
            if (tempBlockPtr->getNext() != 1)
                tempBlockPtr->setNext(i + 1);
            delete tempBlockPtr;
            correct[i] = true;

        }

    }   //every spot in file
    
    delete [] correct;

}

void Defragmenter::reorder(DiskDrive *diskDrive, pair<unsigned short, unsigned short> *parallelDir, unsigned pos, unsigned* startPos, bool* correct) {
    
//diskDrive is original, parallelDir is parallel, pos is where we start sorting, startPos is array of starting positions, correct is whether or not it's handled
    unsigned tempPos = pos, originalSpot = pos;
    DiskBlock* tempBlockPtr = diskDrive->readDiskBlock(tempPos);
    DiskBlock tempBlock = *(tempBlockPtr), currBlock; delete tempBlockPtr;
    pair<unsigned short, unsigned short> tempElement;
    pair<unsigned short, unsigned short> currElement;
    tempElement = parallelDir[tempPos];

    do {
        pos = tempPos;
        tempPos = desiredPos(tempElement, startPos);
        correct[tempPos] = true;

        currElement = tempElement;
        currBlock = tempBlock;
        if (currBlock.getNext() != 1)
            currBlock.setNext(tempPos + 1);
        
        tempBlockPtr = diskDrive->readDiskBlock(tempPos);
        tempBlock = *(tempBlockPtr);
        delete tempBlockPtr;
        tempElement = parallelDir[tempPos];
        
        diskDrive->writeDiskBlock(&currBlock, tempPos);
        parallelDir[tempPos] = currElement;        


    } while (diskDrive->FAT[tempPos] != 0 && tempPos != originalSpot); //haven't reached null, haven't reached original spot




    if (tempPos != originalSpot) { //we stopped at a null, not full cycle
        diskDrive->FAT[originalSpot] = false;
        diskDrive->FAT[tempPos] = true;
    } else { //we replaced the original
        if (tempBlock.getNext() != 1){
            tempBlockPtr = diskDrive->readDiskBlock(tempPos);
            tempBlockPtr->setNext(tempPos + 1);
            delete tempBlockPtr;
        }
    }
}

int Defragmenter::desiredPos(pair<unsigned short, unsigned short> parallelBlock, unsigned* startPos) { //one element of parallelDir

    return startPos[parallelBlock.first] + parallelBlock.second;
}

void Defragmenter::print(pair<unsigned short, unsigned short> *parallelDir, int pos, int tempPos, bool* FAT, pair<unsigned short, unsigned short> tempElement, bool* correct){
    
    for(int i = 2; i < 20; i++)
            cout << parallelDir[i].first << " " << parallelDir[i].second << endl;
    cout << endl << "Pos: " << pos << " tempPos: " << tempPos << endl;
    cout << tempElement.first << " " << tempElement.second << endl; 
    
    cout << "FAT:" << endl;
    for(int i = 2; i < 20; i++)
        cout << FAT[i]<< " ";

	cout << endl;
    
    cout << "correct:" << endl;
    for(int i = 2; i < 20; i++)
        cout << correct[i]<< " ";

        cout << endl;    
    
    
}
