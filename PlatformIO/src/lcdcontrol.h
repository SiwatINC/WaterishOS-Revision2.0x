class disp {
  public:
    LiquidCrystal_I2C lcd(0x27, 16, 2);
    int menu;
    void setmenu(int mnp){
      this->menu = mnp;
    }
    void drawmenu()
    {
      switch(this->menu)
        case: '1':
          this->lcd.print(" Waterish OS OK ");
          
    }
};
