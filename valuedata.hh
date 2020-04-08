class ValueData
{
    public:
        ValueData();
        virtual ~ValueData();

        void setType(int);
        void setType(const char *);
        void setValue(const char *);
        void setValue(const char *, unsigned);

        int getType(void);
        void *getPointer(void);
        
        double decval;
        int intval;
        std::string strval;

    private:
        int type;
};

