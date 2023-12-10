
class UserIntent
{
    public:
        UserIntent();
        void TaskProcess();

    private:


    protected:
    
};

class UserIntentInput
{
    public:
        UserIntentInput();
        virtual void triggerwait() = 0;

    private:
        void Enable() = 0;
        void Disable() = 0;

    protected:
    sem;
};

void SetupButton::SetupButtonTask()
{
    while()
    {
        
    }
}

class SetupButton : public UserIntentInput
{
    public:
        SetupButton(){ createtask(SetupButtonTask); }
        virtual void TriggerWait();
        static SetupButton GetInstance();
        static void SetupButtonTask();

    private:
        void Enable();
        void Disable();

        GpioControl button;

    protected:

};

void Isr()
{
    GetInstance
    starttimer()
}

void Timeout()
{

}

class GpioControl
{
    public:
        GpioControl();
        void Enble();
        void Disble();
        void TriggerWait();

        void Isr();
        void Timeout();

    private:
        uint8_t gpio;
        uint8_t callout;
        uint8_t sem;


    protected:

};
