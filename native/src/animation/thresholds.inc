// Generated original threshold and hold methods.
    bool playTest(float src){
        return defaultThresholds ? 
            ((inVal < 1) && (src >= 1)):
            ((playThresholdDir & ThresholdDir::Up) && (
                inVal < playThresholdValue) && (src >= playThresholdValue) ? 
                true :
                ((playThresholdDir & ThresholdDir::Down) && (
                    inVal > playThresholdValue) && (src <= playThresholdValue)) ?
                    true:
                    false);
    }

    bool stopTest(float src){
        return defaultThresholds ? 
            ((inVal > 0) && (src <= 0)):
            ((stopThresholdDir & ThresholdDir::Up) && (
                inVal < stopThresholdValue) && (src >= stopThresholdValue) ? 
                true :
                ((stopThresholdDir & ThresholdDir::Down) && (
                    inVal > stopThresholdValue) && (src <= stopThresholdValue)) ?
                    true:
                    false);
    }

    bool fullStopTest(float src){
        return defaultThresholds ? 
            ((inVal > -1) && (src <= -1)):
            ((fullStopThresholdDir & ThresholdDir::Up) && (
                inVal < fullStopThresholdValue) && (src >= fullStopThresholdValue) ? 
                true :
                ((fullStopThresholdDir & ThresholdDir::Down) && (
                    inVal > fullStopThresholdValue) && (src <= fullStopThresholdValue)) ?
                    true:
                    false);
    }

    bool playHoldCheck(float src){
        return defaultThresholds ? 
            (src >= 1) :
            ((playThresholdDir & ThresholdDir::Up) && (src >= playThresholdValue) ? 
                true :
                ((playThresholdDir & ThresholdDir::Down) && (src <= playThresholdValue)) ?
                    true :
                    false);
    }

    bool stopHoldCheck(float src){
        return defaultThresholds ? 
            (src <= 0) :
            ((stopThresholdDir & ThresholdDir::Up) && (src >= stopThresholdValue) ? 
                true :
                ((stopThresholdDir & ThresholdDir::Down) && (src <= stopThresholdValue)) ?
                    true :
                    false);
    }

    bool fullStopHoldCheck(float src){
        return defaultThresholds ? 
            (src <= -1) :
            ((fullStopThresholdDir & ThresholdDir::Up) && (src >= fullStopThresholdValue) ? 
                true :
                ((fullStopThresholdDir & ThresholdDir::Down) && (src <= fullStopThresholdValue)) ?
                    true :
                    false);
    }

    bool playHoldTest(float src) {
        return playHoldCheck(src) && playThresholdHoldTime >= playThresholdHoldDelay;
    }

    bool stopHoldTest(float src) {
        return stopHoldCheck(src) && stopThresholdHoldTime >= stopThresholdHoldDelay;
    }

    bool fullStopHoldTest(float src) {
        return fullStopHoldCheck(src) && fullStopThresholdHoldTime >= fullStopThresholdHoldDelay;
    }

    void updateHold(float src){
        if(useHoldDelay && playThresholdHoldDelay > 0) {
            if(playThresholdHoldTime == 0)
            {
                if(playTest(src)) {
                    playThresholdHoldTime += static_cast<int>(nicxlive::core::deltaTime() * 1000);
                }
                else {
                    playThresholdHoldTime = 0;
                }
            }
            else
            {
                if(playHoldCheck(src)) {
                    playThresholdHoldTime += static_cast<int>(nicxlive::core::deltaTime() * 1000);
                }
                else {
                    playThresholdHoldTime = 0;
                }
            }
        }

        if(useHoldDelay && stopThresholdHoldDelay > 0) {
            if(stopThresholdHoldTime == 0)
            {
                if(stopTest(src)) {
                    stopThresholdHoldTime += static_cast<int>(nicxlive::core::deltaTime() * 1000);
                }
                else {
                    stopThresholdHoldTime = 0;
                }
            }
            else
            {
                if(stopHoldCheck(src)) {
                    stopThresholdHoldTime += static_cast<int>(nicxlive::core::deltaTime() * 1000);
                }
                else {
                    stopThresholdHoldTime = 0;
                }
            }
        }

        if(useHoldDelay && fullStopThresholdHoldDelay > 0) {
            if(fullStopThresholdHoldTime == 0)
            {
                if(fullStopTest(src)) {
                    fullStopThresholdHoldTime += static_cast<int>(nicxlive::core::deltaTime() * 1000);
                }
                else {
                    fullStopThresholdHoldTime = 0;
                }
            }
            else
            {
                if(fullStopHoldCheck(src)) {
                    fullStopThresholdHoldTime += static_cast<int>(nicxlive::core::deltaTime() * 1000);
                }
                else {
                    fullStopThresholdHoldTime = 0;
                }

            }
        }
    }

    bool playEventTest(TriggerEvent event){
        return static_cast<bool> (event & playEvent);
    }

    bool stopEventTest(TriggerEvent event){
        return static_cast<bool> (event & stopEvent);
    }

    bool fullStopEventTest(TriggerEvent event){
        return static_cast<bool> (event & fullStopEvent);
    }
