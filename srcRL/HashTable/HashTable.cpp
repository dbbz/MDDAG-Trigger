//#include "RBFQETraces.h"
#include "HashTable.h"
#include "HashTableStateModifier.h"
#include "ExampleResults.h"

//#include "RBFStateModifier.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//CAbstractQETraces* GSBNFBasedQFunction::getStandardETraces()
//{
//    return new GSBNFQETraces(this);
//}
//


// -----------------------------------------------------------------------------------

HashTable::HashTable(CActionSet *actions, CStateModifier* sm,  MultiBoost::AdaBoostMDPClassifierContinous* classifier, int numDim) : CAbstractQFunction(actions)
{
    _stateProperties = sm;
    _classifier = classifier;
    _actions = actions;
    _numberOfActions = (int)actions->size();
    
    addParameter("QLearningRate", 0.2);
    
    _numDimensions = numDim;
    if (_numDimensions == 2) --_numDimensions;
    
    _numDimensions = 1;
}

// -----------------------------------------------------------------------------------

double HashTable::getValue(CStateCollection* state, CAction *action, CActionData *data) {
    int actionIndex = dynamic_cast<MultiBoost::CAdaBoostAction*>(action)->getMode();
    ValueKey key;
    MDDAGState mddagState(state->getState());
    getKey(mddagState, key);
    AlphaReal value;
    getTableValue(actionIndex, key, value);
    return value;
};

// -----------------------------------------------------------------------------------

bool HashTable::getTableValue(int actionIndex, ValueKey& key, AlphaReal& outValue, AlphaReal defaultValue)
{
    ValueTableType::const_iterator it = _valueTable.find(key);
    if (it == _valueTable.end()) {
        outValue = defaultValue;
        return false;
    }
    else {
        outValue = it->second[actionIndex];
        return true;
    }
}

// -----------------------------------------------------------------------------------

void HashTable::getKey(MDDAGState& state, ValueKey& key)
{    
    //        vector<int> history;
    //        _classifier->getHistory( history );
    //        _classifier->getClassifiersOutput(history);
    
    //        const size_t numDimensions = 0;// currState->getNumActiveContinuousStates();
//    size_t numDimensions = state.continuousStates.size();


//    numDimensions = _numDimensions;
    

//    vector<int> history = _classifier->getHistoryFromState( state.discreteStates[1] );
    
//    for (auto it = history.rbegin(); it != history.rend(); ++it) {
//        if (*it == 0.) history.pop_back();
//        else break;
//    }
    
//    size_t numEvaluations = history.size();
    
    
    vector<int> winners = _classifier->getWinnersFromState( state.discreteStates[2] );

    key.clear();
//    key.resize(numDimensions + numEvaluations + 1);//+ 1
    
    const int numWinners = 2;
    // index + numWinners + difference between the first two winners
    key.resize(1 + numWinners + 1);//+ 1
    
    int i = 0;
    key[i++] = state.discreteStates[0];
    
//    const MultiBoost::ExampleResults* examplesResult = _classifier->getCurrentExampleResults();
//    AlphaReal scoreDifference = (examplesResult->getWinner(0).second - examplesResult->getWinner(1).second) / 2;

//    for (const auto & myTmpKey : state.continuousStates) cout << myTmpKey << " "; cout << endl;
//    for (const auto & myTmpKey : winners) cout << myTmpKey << " "; cout << endl;
    AlphaReal scoreDifference = (state.continuousStates[winners[0]] - state.continuousStates[winners[1]]) / 2;

    const int numPartitions = 9;
    const AlphaReal step = 1. / (numPartitions);

    int p = int(scoreDifference / step);
    
    key[i++] = p;

//    cout << "+++[DEBUG] examplesResult->getWinner(0).second " << examplesResult->getWinner(0).second << endl;
//    cout << "+++[DEBUG] examplesResult->getWinner(1).second " << examplesResult->getWinner(1).second << endl;
//    cout << "+++ " << endl;
//    
    for (int j = 0; j < numWinners; ++j) {
//        int winner = examplesResult->getWinner(j).first;
        key[i++] = winners[j];
    }
    
    
//    for (int j = 0; j < numDimensions; ++i, ++j) {
//        
//        // rounding operation
//        //            int score = (int)(currState->getContinuousState(j) * 1000);
//        //            key[i] = score;
//        key[i] = state.continuousStates[j];
//    }
    
    
//    for (int k = 0; k < numEvaluations; ++i, ++k) {
//        key[i] = history[k]; 
//    }
    
}


// -----------------------------------------------------------------------------------

double HashTable::getMaxValue(MDDAGState& state)
{
    ValueKey key;
    getKey(state, key);
    
    AlphaReal maxVal = 0.0;
    
    for( int i=0; i < _numberOfActions; ++i )
    {
        AlphaReal value;
        getTableValue(i, key, value);
        if (value > maxVal) {
            maxVal = value;
        }
    }
    return maxVal;
    cout << endl << "max activation " << maxVal << endl;
}

// -----------------------------------------------------------------------------------

void HashTable::addTableEntry(double tderror, ValueKey& key, int actionIndex)
{    
    //        cout << "+++[DEBUG] new entry: " ;
    //        for (int i = 0; i < key.size(); ++i) {
    //            cout << key[i] << flush;
    //        }
    //        cout << endl;
    
    //        cout << "+++[DEBUG] New entry: " ;
    //        for (int i = 0; i < key.size(); ++i) {
    //            cout << key[i] << " ";
    //        }
    //        cout << endl;
    
//    ValueTableType::const_iterator it = _valueTable.find(key);    
//    assert(it == _valueTable.end());
//    
    _valueTable[key].resize(_numberOfActions);
    _valueTable[key][actionIndex] = tderror;
}

// -----------------------------------------------------------------------------------

void HashTable::updateValue(MDDAGState& state, CAction *action, double td, CActionData * actionData)
{
    if (td != td) {
        assert(false);
    }
    
    int actionIndex = dynamic_cast<MultiBoost::CAdaBoostAction*>(action)->getMode();
    
    ValueKey key;
    getKey(state, key);
    
    AlphaReal value;
    bool entryExists = getTableValue(actionIndex, key, value);
    
    if (entryExists)
        _valueTable[key][actionIndex] += td; //* _learningRate;
    else
        addTableEntry(td, key, actionIndex);
};

// -----------------------------------------------------------------------------------

void HashTable::saveActionValueTable(FILE* stream, int dim)
{
    //        fprintf(stream, "Q-Hash Table\n");
    
    ValueTableType::iterator tableIt = _valueTable.begin();
    
    for (; tableIt != _valueTable.end(); ++tableIt) {
        
        ValueKey key = tableIt->first;
        vector<AlphaReal> values = tableIt->second;
        
        fprintf(stream, "( ");
        ValueKey::iterator keyIt = key.begin();
        if (keyIt != key.end()) fprintf(stream, "%d ", (int)*(keyIt++));
        
//        for (int d = 0; d < _numDimensions; ++d, ++keyIt) {
//            fprintf(stream, "%f ", ((*keyIt)*2) - 1);
//        }
        
        if (keyIt != key.end()) fprintf(stream, "%d ", (int)*(keyIt++));
        fprintf(stream, "  ");

        for (; keyIt != key.end(); ++keyIt) {
            fprintf(stream, "%d ", (int)(*keyIt));
        }
        
        fprintf(stream, ")\t");
        
        for (int i = 0; i < values.size(); ++i) {
            fprintf(stream, "%f ", values[i]);
        }
        
        fprintf(stream, "\n");
    }
    
}

// -----------------------------------------------------------------------------------

