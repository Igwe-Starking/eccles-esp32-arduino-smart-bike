//Base implementation of executormanager

#include "Executors.h"

ECCLES_API {

    //prepare any executor that need preparation
    void ExecutorManager::prepare(){
        //initialize the executor mutex
        Executor::start(); //very very important or else all obtain and send will fail silently
        //prepare configuration
        cnf.open();
    }

    //void sets executors result handler
    void ExecutorManager::setResultHandler(ResultHandler* h){
        Executor::setHandler(h);
    }

    //run the executor command pool
    void ExecutorManager::run(){
        Executor::checkIncomingCommands();
        dv.runMonitor();
    }

    //cleanup any executor that needs cleanup
    void cleanup(){
        //i don't think we need this am still checking wether to remove this
    }
};