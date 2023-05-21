#include <iostream>
#include <cmath>
#include <cstring>

using namespace std;

// Перечисление возможных наименований фигур
enum piece_name {Pawn, Knight = 8, Bishop = 10, Rook = 12, Queen = 14, King = 15, NoName};
// Перечисление цветов
enum piece_colour {White, Black = 16, NoColour};
// Нумерация введена для размещения адресов фигур в массиве Game.PiecePointer

const int Gridsize = 8; // Размер поля

// Начальная позиция, записанная в формате FEN
char startFEN[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq";
//char startFEN[] = "4k3/3R1R2/8/8/4P3/8/PPPP1PPP/1N1K1N1 b KQkq";

// Шаги указателя по двумерному массиву клеток
const int StepUp = 1;
const int StepDown = -1;
const int StepLeft = -Gridsize;
const int StepRight = Gridsize;

// Объединение для записи направления в случае шаха или блокировки
union pinstep {int TwoBytes; char OneByte[2];};

struct king_safety;
struct info;

// Класс - клетка
class cell {
    piece_name Name; // Наименование фигуры, занимающей клетку
    piece_colour Colour; // Цвет фигуры, занимающей клетку
    int Vertical_Coord; // Вертикальная координата
    int Horizontal_Coord; // Горизонтальная координата
    
    void writesquare (char *PieceMoves, int mode); // Короткая функция записи координат в строку доступных ходов
    
    // Функция, записывающая все возможные ходы для фигуры на клетке в Game.CorrectMoves
    void movement_list (); 
    // Функции типа ..._moves вызываются из movement_list() и отвечают за возможные ходы одной фигуры с одной клетки
    void bishop_moves (char *PieceMoves); // Функция проверки и записи ходов по диагоналям
    void rook_moves (char *PieceMoves); // Функция проверки и записи ходов по горизонтали и вертикали
    void king_moves (char *PieceMoves); // Функция проверки ходов короля
    bool squarecheck (char *PieceMoves); // Функция проверки конкретной клетки
    
    // Функция, проверяющая клетку короля на предмет атаки вражескими фигурами
    // При вызове без аргумента возвращает false при первом обнаружении угрозы
    // При вызове с аргументом 1 также учитывает состояние шаха и блокированные фигуры и корректирует Game.CorrectMoves
    bool check_king (int mode = 0);
    
    // Функции типа ..._threat вызываются из check_king() и отвечают за проверку клетки на нахождение на ней опасности
    int king_threat (int mode); // Проверка вражеского короля
    int rook_threat (int mode, int step, king_safety& SafetyInfo); // Проверка вражеской ладьи и ферзя
    int bishop_threat (int mode, int step, king_safety& SafetyInfo); // Проверка вражеского слона и ферзя
    int pawn_threat (int mode, int step, king_safety& SafetyInfo); // Проверка вражеской пешки
    int knight_threat (int mode, king_safety& SafetyInfo); // Проверка вражеского коня
    
    
  public:
    void set_cell ( int hor, int vert, piece_name name, piece_colour colour); // Определение всех параметров клетки
    void pointer_set (); // Запись адреса клетки с фигурой в массив Game.PiecePointer
    
    void operator=(cell& Initial); // Перегруженный оператор "=", используемый для осуществления хода
    
    char board_symbol(); // Вывод в консоль символа, соответствующего наименованию фигуры
    
    friend bool count_moves(); // Расчет доступных ходов и проверка конца игры
    friend bool read_command(char* command); // Проверка правильности команды и совершение хода
    
    friend void check_castle(); // Проверка доступности рокировок
    
    friend void load_FEN (char* Position); // Загрузка партии по нотации FEN
};

// Структура, хранящая все необходимые данные, относящиеся к партии
struct info {
    piece_colour CurrentColour = White; // Цвет фигур игрока, делающего текущий ход
    int TurnCount = 1; // Счетчик ходов (пока не использован)
    char MoveLog[1000] = ""; // История ходов (пока не использован)
    
    // Строка, содержащая все доступные ходы
    // Формат записи : ... /e4:e5e6 ..., где e4 -  исходная клетка; e5,e6 - доступные к перемещению клетки
    char CorrectMoves[720] = ""; 
    
    bool GameOver = false; // Флаг окончания игры
    
    // Флаги доступности рокировок
    bool WhiteLongCastleAvailable = false;
    bool WhiteShortCastleAvailable = false;
    bool BlackLongCastleAvailable = false;
    bool BlackShortCastleAvailable = false;
    
    // Массив Адресов всех фигур, порядок соотетствует нумерации перечислений
    cell *PiecePointer[32];
    
    // Конструктор, устанавливающий адреса фигур нулевыми
    info () {for (int i = 0; i < 32; i++) PiecePointer[i] = 0;}
};

// Создание глобальной структуры Game и массива клеток Board 8x8
info Game;
cell Board[Gridsize][Gridsize];

// Структура, хранящая все данные, необходимые при проверке безопасности короля
struct king_safety {
    int Attackers = 0;  //Количество фигур, дающих шах королю
    cell* Ally = 0;     // Указатель на клетку-союзника, блокирующего шах
    
    // Строка, содержащая положение блокированных фигур
    /* Формат записи : ... e6vhe2 ..., где e6 - клетка блокированной фигуры; h и v - (-1, 1) шаги по вертикали и горизонтали;
    e2 - клетка атакующей фигуры */
    char Pinned[55] = "";
    
    // Строка, содержащая положение фигуры, дающей шах королю
    /* Формат записи : e6vhe1, где e6 - клетка атакующего; h и v - (-1, 1) шаги по вертикали и горизонтали;
    e1 - клетка короля*/
    char Threats[33] = "";
};

// Определение всех параметров клетки
void cell :: set_cell (int hor, int vert, piece_name n = NoName, piece_colour c = NoColour)
{
    Name = n;
    Colour = c;
    Vertical_Coord = vert;
    Horizontal_Coord = hor;
}

// Встраиваемая функция записи координат в строку доступных ходов
inline void cell :: writesquare (char *PieceMoves, int mode = 0)
{
    char temp[3];
    temp[0] = 'a' + char(Horizontal_Coord);
    temp[1] = '1' + char(Vertical_Coord);
    
    if (mode == 0)
        // Запись координат в строку, не содержащую нулевых символов, т.е. Game.CorrectMoves
        strcat(PieceMoves, temp);
    else{
        // Запись координат в строку, содержащую нулевой символ, т.е. Pinned и Threats
        for (; *PieceMoves || *(PieceMoves + 1); PieceMoves += 2);
        *(PieceMoves++) = 'a' + char(Horizontal_Coord);
        *PieceMoves = '1' + char(Vertical_Coord);
    }
}

// Встраиваемая функция, записывающая направление от исходной клетки к конечной
// Используется для записи в Pinned и Threats
// Записывает два char значения, при сложении с которыми получается на шаг приближенные к конечным координаты
// Например 'e' + 0b0000 = 'e', '4' + 0b1111 = '3': "e4" -> "e3"
inline void writestep (char* Pinned, int step)
{
    pinstep s;
    
    switch (step){
        case StepUp:
            s.TwoBytes = 1; break; // 0000 0001
        case StepDown:
            s.TwoBytes = 255; break; // 0000 1111
        case StepLeft:
            s.TwoBytes = -256; break; // 1111 0000
        case StepRight:
            s.TwoBytes = 256; break; // 0001 0000
        case StepUp + StepRight:
            s.TwoBytes = 257; break; // 0001 0001
        case StepUp + StepLeft:
            s.TwoBytes = -255; break; // 1111 0001
        case StepDown + StepRight:
            s.TwoBytes = 511; break; // 0001 1111
        case StepDown + StepLeft:
            s.TwoBytes = -1; break; // 1111 1111
    }
    
    for (; *(Pinned) || *(Pinned + 1); Pinned += 2); // Поиск конца в строке, возможно содержащей нулевой символ
    
    *(Pinned++) = s.OneByte[1];
    *Pinned = s.OneByte[0];

    return;
}

// Встраиваемая функция, проверяющая цвет клетки. Используется при расчете всех возможных ходов
inline bool cell :: squarecheck(char *PieceMoves)
{
    if (Colour == NoColour){
        writesquare(PieceMoves);
        return true;
    }
    
    if (Colour == Game.CurrentColour)
        return false;
    
    if (Colour != Game.CurrentColour)
        writesquare(PieceMoves);
    
    return false;
}

// Запись всех возможных ходов по диагоналям из исходной клетки
void cell :: bishop_moves (char *PieceMoves)
{
    cell *TargetSquare = 0;
    int step;
    
    if (Vertical_Coord < 7 && Horizontal_Coord > 0){
        TargetSquare = this + StepUp + StepLeft;
        for (step = StepUp + StepLeft; ; TargetSquare += step){
            if (TargetSquare -> squarecheck (PieceMoves) == false)
                break;
            if (TargetSquare -> Vertical_Coord == 7 || TargetSquare -> Horizontal_Coord == 0)
                break;
        }
    }
            
    if (Vertical_Coord < 7 && Horizontal_Coord < 7){
        TargetSquare = this + StepUp + StepRight;
        for (step = StepUp + StepRight; ; TargetSquare += step){
            if (TargetSquare -> squarecheck (PieceMoves) == false)
                break;
            if (TargetSquare -> Vertical_Coord == 7 || TargetSquare -> Horizontal_Coord == 7)
                break;
        }
    }
            
    if (Vertical_Coord > 0 && Horizontal_Coord > 0){
        TargetSquare = this + StepDown + StepRight;
        for (step = StepDown + StepRight; ; TargetSquare += step){
            if (TargetSquare -> squarecheck (PieceMoves) == false)
                break;
            if (TargetSquare -> Vertical_Coord == 0 || TargetSquare -> Horizontal_Coord == 7)
                break;
        }
    }
            
    if (Vertical_Coord > 0 && Horizontal_Coord < 7){
        TargetSquare = this + StepDown + StepLeft;
        for (step = StepDown + StepLeft; ; TargetSquare += step){
            if (TargetSquare -> squarecheck (PieceMoves) == false)
                break;
            if (TargetSquare -> Vertical_Coord == 0 || TargetSquare -> Horizontal_Coord == 0)
                break;
        }
    }
}

// Запись всех возможных ходов по вертикали и горизонтали из исходной клетки
void cell :: rook_moves (char *PieceMoves)
{
    cell *TargetSquare = 0;
    cell *SquareZero = &Board[0][0];
    cell *SquareEnd = &Board[7][7];
    int step;
    
    TargetSquare = this + StepLeft;
    for (step = StepLeft; TargetSquare >= SquareZero; TargetSquare += step){
        if (TargetSquare -> squarecheck (PieceMoves) == false)
                break;
    }
    
    TargetSquare = this + StepRight;
    for (step = StepRight; TargetSquare <= SquareEnd; TargetSquare += step){
        if (TargetSquare -> squarecheck (PieceMoves) == false)
                break;
    }
    
    if (Vertical_Coord < 7){
        TargetSquare = this + StepUp;
        for (step = StepUp; TargetSquare <= SquareEnd; TargetSquare += step){
            if (TargetSquare -> squarecheck (PieceMoves) == false)
                break;
            if (TargetSquare -> Vertical_Coord == 7)
                break;
        }
    }
    
    if (Vertical_Coord > 0){
        TargetSquare = this + StepDown;
        for (step = StepDown; (TargetSquare -> Vertical_Coord) >= 0 && TargetSquare >= SquareZero; TargetSquare += step){
            if (TargetSquare -> squarecheck (PieceMoves) == false)
                break;
            if (TargetSquare -> Vertical_Coord == 7)
                break;
        }
    }
}

// Запись всех возможных ходов короля из исходной клетки
void cell :: king_moves (char *PieceMoves)
{
    cell *TargetSquare = 0;
    cell *SquareZero = &Board[0][0];
    cell *SquareEnd = &Board[7][7];
    
    // При проверке ходов короля на каждом шаге проверяется безопасность короля
    // Во избежание "защиты" короля им самим, на время проверки его клетка обнуляется
    Colour = NoColour;
    Name = NoName;
    
    if (Horizontal_Coord > 0){
        TargetSquare = this + StepLeft;
        
        if (TargetSquare -> Colour != Game.CurrentColour)
            if (TargetSquare -> check_king())
                TargetSquare -> writesquare(PieceMoves);
    }
    
    if (Horizontal_Coord < 7){
        TargetSquare = this + StepRight;
        
        if (TargetSquare -> Colour != Game.CurrentColour)
            if (TargetSquare -> check_king())
                TargetSquare -> writesquare(PieceMoves);
    }
    
    if (Vertical_Coord < 7){
        TargetSquare = this + StepUp;
        
        if (TargetSquare -> Colour != Game.CurrentColour)
            if (TargetSquare -> check_king())
                TargetSquare -> writesquare(PieceMoves);
    }
    
    if (Vertical_Coord > 0){
        TargetSquare = this + StepDown;
        
        if (TargetSquare -> Colour != Game.CurrentColour)
            if (TargetSquare -> check_king())
                TargetSquare -> writesquare(PieceMoves);
    }
    
    if (Vertical_Coord > 0 && Horizontal_Coord > 0){
        TargetSquare = this + StepDown + StepLeft;
        
        if (TargetSquare -> Colour != Game.CurrentColour)
            if (TargetSquare -> check_king())
                TargetSquare -> writesquare(PieceMoves);
    }
    
    if (Vertical_Coord > 0 && Horizontal_Coord < 7){
        TargetSquare = this + StepDown + StepRight;
        
        if (TargetSquare -> Colour != Game.CurrentColour)
            if (TargetSquare -> check_king())
                TargetSquare -> writesquare(PieceMoves);
    }
    
    if (Vertical_Coord < 7 && Horizontal_Coord > 0){
        TargetSquare = this + StepUp + StepLeft;
        
        if (TargetSquare -> Colour != Game.CurrentColour)
            if (TargetSquare -> check_king())
                TargetSquare -> writesquare(PieceMoves);
    }
    
    if (Vertical_Coord < 7 && Horizontal_Coord < 7){
        TargetSquare = this + StepUp + StepRight;
        
        if (TargetSquare -> Colour != Game.CurrentColour)
            if (TargetSquare -> check_king())
                TargetSquare -> writesquare(PieceMoves);
    }
    
    // "Возврат" короля на доску
    Colour = Game.CurrentColour;
    Name = King;
}

// Проверка доступности рокировок и запись соответствующих команд в Game.CorrectMoves
void check_castle ()
{
    if (Game.CurrentColour == White && Game.WhiteShortCastleAvailable){
        if (Board[5][0].Name == NoName && Board[6][0].Name == NoName)
            if (Board[5][0].check_king() && Board[6][0].check_king())
                strcat(Game.CorrectMoves, "/O-O");
    }
    if (Game.CurrentColour == White && Game.WhiteLongCastleAvailable){
        if (Board[3][0].Name == NoName && Board[2][0].Name == NoName)
            if (Board[3][0].check_king() && Board[2][0].check_king())
                strcat(Game.CorrectMoves, "/O-O-O");
    }
    
    if (Game.CurrentColour == Black && Game.BlackShortCastleAvailable){
        if (Board[5][7].Name == NoName && Board[6][7].Name == NoName)
            if (Board[5][7].check_king() && Board[6][7].check_king())
                strcat(Game.CorrectMoves, "/O-O");
    }
    if (Game.CurrentColour == Black && Game.BlackLongCastleAvailable){
        if (Board[3][7].Name == NoName && Board[2][7].Name == NoName)
            if (Board[3][7].check_king() && Board[2][7].check_king())
                strcat(Game.CorrectMoves, "/O-O-O");
    }
}

// Вычисление и запись всех доступных ходов с клетки
void cell :: movement_list ()
{
    char Headline[5] = "";
    char PieceMoves[43] = "";
    cell *TargetSquare = this;
    cell *SquareZero = &Board[0][0];
    cell *SquareEnd = &Board[7][7];
    int step;
    
    // Заголовок формата "/е4:"
    Headline[0] = '/';
    writesquare(Headline);
    Headline[3] = ':';
    
    switch (Name){
        case Pawn:
            if (Colour == White)
                step = StepUp;
            else
                step = StepDown;
            
            TargetSquare = this + step;
            if (TargetSquare -> Colour == NoColour){
                TargetSquare -> writesquare(PieceMoves);
                
                TargetSquare += step;
                if (Vertical_Coord == 1 && Colour == White && TargetSquare -> Colour == NoColour)
                    TargetSquare -> writesquare(PieceMoves);
                
                if (Vertical_Coord == 6 && Colour == Black && TargetSquare -> Colour == NoColour)
                    TargetSquare -> writesquare(PieceMoves);
            }
            
            if (Horizontal_Coord != 0){
                if (Colour == White)
                    step = StepUp + StepLeft;
                else
                    step = StepDown + StepLeft;
                
                TargetSquare = this + step;
                if (TargetSquare -> Colour != Colour && TargetSquare -> Colour != NoColour)
                    TargetSquare -> writesquare(PieceMoves);
            }
            
            if (Horizontal_Coord != 7){
                if (Colour == White && Horizontal_Coord < 0)
                    step = StepUp + StepRight;
                else
                    step = StepDown + StepRight;
                
                TargetSquare = this + step;
                if (TargetSquare -> Colour != Colour && TargetSquare -> Colour != NoColour)
                    TargetSquare -> writesquare(PieceMoves);
            }
        break;
        
        case Knight:
            if (Vertical_Coord > 0 && Horizontal_Coord > 1){ TargetSquare = this + 2*StepLeft + StepDown;
                if (TargetSquare -> Colour != Colour) TargetSquare -> writesquare(PieceMoves); }
            
            if (Vertical_Coord > 0 && Horizontal_Coord < 6){ TargetSquare = this + 2*StepRight + StepDown;
                if (TargetSquare -> Colour != Colour) TargetSquare -> writesquare(PieceMoves);}
            
            if (Vertical_Coord < 7 && Horizontal_Coord > 1){ TargetSquare = this + 2*StepLeft + StepUp;
                if (TargetSquare -> Colour != Colour) TargetSquare -> writesquare(PieceMoves);}
            
            if (Vertical_Coord < 7 && Horizontal_Coord < 6){ TargetSquare = this + 2*StepRight + StepUp;
                if (TargetSquare -> Colour != Colour) TargetSquare -> writesquare(PieceMoves);}
            
            if (Vertical_Coord > 1 && Horizontal_Coord > 0){ TargetSquare = this + StepLeft + 2*StepDown;
                if (TargetSquare -> Colour != Colour) TargetSquare -> writesquare(PieceMoves);}
            
            if (Vertical_Coord > 1 && Horizontal_Coord < 7){ TargetSquare = this + StepRight + 2*StepDown;
                if (TargetSquare -> Colour != Colour) TargetSquare -> writesquare(PieceMoves);}
            
            if (Vertical_Coord < 6 && Horizontal_Coord > 0){ TargetSquare = this + StepLeft + 2*StepUp;
                if (TargetSquare -> Colour != Colour) TargetSquare -> writesquare(PieceMoves);}
            
            if (Vertical_Coord < 6 && Horizontal_Coord < 7){ TargetSquare = this + StepRight + 2*StepUp;
                if (TargetSquare -> Colour != Colour) TargetSquare -> writesquare(PieceMoves);}
        break;
        
        case Bishop:
            bishop_moves(PieceMoves);
            break;
        
        case Rook:
            rook_moves(PieceMoves);
            break;
        
        case Queen:
            bishop_moves(PieceMoves);
            rook_moves(PieceMoves);
            break;
        
        case King:
            king_moves(PieceMoves);
            break;
    }
    
    //cout << Headline << PieceMoves << '\n';
    
    // Внесение заголовка в Game.CorrectMoves происходит только при наличии доступных ходов
    if (strcmp(PieceMoves, "")){
        strcat(Game.CorrectMoves, Headline);
        strcat(Game.CorrectMoves, PieceMoves);
    }
}

// Перегруженный оператор "=", используемый для осуществления хода
void cell :: operator=(cell& Initial)
{
    // Левая граница индекса фигуры в массиве Game.PiecePointer
    // Например: черный слон ищется в массиве начиная с индекса 16 + 10 = 26
    int i = (int) Game.CurrentColour + (int) Initial.Name; 
    
    int count; // Количество фигур с таким же именем в массиве Game.PiecePointer
    
    switch(Initial.Name){
        case Pawn: count = 8; break;
        case King: count = 1; break;
        case Queen: count = 1; break;
        default: count = 2; break;
    }
    
    // Замена адреса клетки, на которой стояла фигура, адресом клетки, на которую она передвинулась
    for(; count > 0; count--, i++)
        if (Game.PiecePointer[i] == &Initial)
            Game.PiecePointer[i] = this;
    
    // Если фигура забирает вражескую, адрес, соответствующий вражеской фигуре, в Game.PiecePointer зануляется
    if (Name != NoName){
        i = (int) Colour + (int) Name;
        switch(Name){
            case Pawn: count = 8; break;
            case King: count = 1; break;
            case Queen: count = 1; break;
            default: count = 2; break;
        } // Аналогичный поиск вражеской фигуры в массиве адресов
    
        for(; count > 0; count--, i++)
            if (Game.PiecePointer[i] == this)
                Game.PiecePointer[i] = 0;
    }
    
    Name = Initial.Name;
    Colour = Initial.Colour; // На конечной клетке в итоге оказывается фигура с начальной
    
    Initial.Name = NoName;
    Initial.Colour = NoColour; // Начальная клетка в итоге окаызывается свободна
    
}

// Вывод в консоль символа, соответствующего наименованию фигуры
char cell :: board_symbol()
{
    switch (Name){
        case NoName: return 'o';
        case Pawn: return 'p';
        case Knight: return 'n';
        case Bishop: return 'b';
        case Rook: return 'r';
        case Queen: return 'Q';
        case King: return 'K';
    }
    return 'o';
}

// Запись адреса клетки с фигурой в массив Game.PiecePointer
inline void cell :: pointer_set ()
{
    int count = (int) Colour + (int) Name;
    while (Game.PiecePointer[count])
        count++;
    Game.PiecePointer[count] = this;
}

// Загрузка позиции в нотации FEN
void load_FEN (char* sym)
{
    cell* const StartSquare = &Board[0][7]; // Расположение фигур считывается начиная с клетки а8
    cell* Square = StartSquare;
    int spaces, rowcount;
    
    // Считывание положения фигур
    for (rowcount = 1; rowcount < 9; ++sym){
        
        // Обнаружение цифры и пропуск пробелов
        if (*sym > '0' && *sym < '9'){
            spaces = *sym - '0';
            for (; spaces > 0; spaces--, Square += StepRight);
        }
        else{
            switch (*sym){
                case 'R': Square -> Name = Rook; Square -> Colour = White; break;
                case 'N': Square -> Name = Knight; Square -> Colour = White; break;
                case 'B': Square -> Name = Bishop; Square -> Colour = White; break;
                case 'Q': Square -> Name = Queen; Square -> Colour = White; break;
                case 'K': Square -> Name = King; Square -> Colour = White; break;
                case 'P': Square -> Name = Pawn; Square -> Colour = White; break;
                
                case 'r': Square -> Name = Rook; Square -> Colour = Black; break;
                case 'n': Square -> Name = Knight; Square -> Colour = Black; break;
                case 'b': Square -> Name = Bishop; Square -> Colour = Black; break;
                case 'q': Square -> Name = Queen; Square -> Colour = Black; break;
                case 'k': Square -> Name = King; Square -> Colour = Black; break;
                case 'p': Square -> Name = Pawn; Square -> Colour = Black; break;
            }
            if (*sym == '/' || *sym == ' '){
                Square = StartSquare - rowcount; // Переход на следующий ряд
                rowcount++;
                continue;
            }
            else{
                Square -> pointer_set();
                
                if (Square -> Horizontal_Coord < 7)
                    Square += StepRight;
            }
        }
    }
    
    // Считывание активного цвета
    if (*(sym) == 'w')
        Game.CurrentColour = White;
    if (*(sym ) == 'b')
        Game.CurrentColour = Black;
    
    sym += 2;
    
    // Считывание доступных рокировок
    for (; *sym; sym++){
        switch (*sym){
            case 'K': Game.WhiteShortCastleAvailable = true; break;
            case 'Q': Game.WhiteLongCastleAvailable = true; break;
            case 'k': Game.BlackShortCastleAvailable = true; break;
            case 'q': Game.BlackLongCastleAvailable = true; break;
        }
    }
}

// Вывод в консоль всей доски
void show_board()
{
    int v, h;
    
    for (v = 7; v >= 0; --v){
        for (h = 0; h < 8; ++h)
            cout << Board[h][v].board_symbol() << ' ';
        cout << '\n';
    }
}

// Инициализация доски - назначение всем клеткам их координат
void init_board()
{
    int i, j;

    for (j = 0; j < 8; ++j)
        for (i = 0; i < 8; ++i)
            Board[i][j].set_cell(i, j);
}

// Осуществление рокировок в короткую сторону
void short_castle()
{
    if (Game.CurrentColour == White){
        Board[6][0] = Board[4][0];
        Board[5][0] = Board[7][0];
        Game.WhiteShortCastleAvailable = false;
        Game.WhiteLongCastleAvailable = false;
        return;
    }
    
    if (Game.CurrentColour == Black){
        Board[6][7] = Board[4][7];
        Board[5][7] = Board[7][7];
        Game.BlackShortCastleAvailable = false;
        Game.BlackLongCastleAvailable = false;
        return;
    }
}

// Осуществление рокировок в длинную сторону
void long_castle()
{
    if (Game.CurrentColour == White){
        Board[2][0] = Board[4][0];
        Board[3][0] = Board[0][0];
        Game.WhiteShortCastleAvailable = false;
        Game.WhiteLongCastleAvailable = false;
        return;
    }
    
    if (Game.CurrentColour == Black){
        Board[2][7] = Board[4][7];
        Board[3][7] = Board[0][7];
        Game.BlackShortCastleAvailable = false;
        Game.BlackLongCastleAvailable = false;
        return;
    }
}

// Функция, заменяющая массив Game.CorrectMoves на аналогичный, но только с ходами, нейтрализующими шах королю
void threat_block (char* Threats)
{
    char NewCorrectMoves[720] = "";
    char Headline[5] = "/  :";
    char PieceMoves[15] = "";
    char TargetSquare[3] = "";
    char* HeadPointer = Game.CorrectMoves;
    char* TargetPointer;
    char* end;
    
    // Поиск конца
    for (end = Game.CorrectMoves; *end; ++end);
    
    while (HeadPointer < end){
        for (; *HeadPointer != '/'; ++HeadPointer); // Поиск исходной клетки, например "/e4:"
        
        Headline[1] = *(HeadPointer + 1);
        Headline[2] = *(HeadPointer + 2);
        HeadPointer += 4;
        
        TargetSquare[0] = Threats[0];
        TargetSquare[1] = Threats[1];
        
        while (TargetSquare[0] != Threats[4] || TargetSquare[1] != Threats[5]){
        /* В цикле проверяют наличие поочередно всех клеток между королем и атакующим включительно, совпадения
        записываются в PieceMoves.*/
            
            for (TargetPointer = HeadPointer; *TargetPointer != '/' && TargetPointer <= end; TargetPointer += 2){
                if (*TargetPointer == TargetSquare[0] && *(TargetPointer + 1) == TargetSquare[1])
                    strcat(PieceMoves, TargetSquare);
            }
            
            if (Threats[2] == 'K' && Threats[3] == 'n')
                break;
            
            if (Threats[2] == 'P' && Threats[3] == 'w')
                break;
            
            TargetSquare[0] += Threats[2];
            TargetSquare[1] += Threats[3];
        }
        
        // Если найдено хоть одно совпадение, в NewCorrectMoves записывается заголовок и все совпадения
        if (strcmp(PieceMoves, "")){
            strcat(NewCorrectMoves, Headline);
            strcat(NewCorrectMoves, PieceMoves);
        }
        
        HeadPointer = TargetPointer; // Переход к следующему заголовку
        for (char* p = PieceMoves; *p; p++) *p = '\0'; // Очистка PieceMoves
    }
    
    strcpy (Game.CorrectMoves, NewCorrectMoves);
}

// Функция, исключающая из Game.CorrectMoves все ходы заблокированной фигуры, кроме ходов навстречу атакующему
void pinned_exclude (char* Pinned)
{
    char TargetSquare[3];
    char *leftcount, *rightcount;
    
    // Поиск координат заблокированной фигуры
    for (leftcount = Game.CorrectMoves; *(leftcount + 1); ++leftcount){
        if (*leftcount == '/'){
            if (*(leftcount + 1) == Pinned[0] && *(leftcount + 2) == Pinned[1]){
                leftcount += 4;
                rightcount = leftcount;
                goto initalfound;
            }
            else
                leftcount += 3;
        }
    }
    return;

initalfound:
    
    TargetSquare[0] = Pinned[0];
    TargetSquare[1] = Pinned[1];
    while (TargetSquare[0] != Pinned[4] || TargetSquare[1] != Pinned[5]){
        
        TargetSquare[0] += Pinned[2];
        TargetSquare[1] += Pinned[3];
        
        // Цикл проходит все доступные ходы фигуры и сравнивает их с клетками по направлению к атакующему
        for (; *rightcount != '/' && *rightcount != '\0'; rightcount += 2){
            if (*rightcount == TargetSquare[0] && *(rightcount + 1) == TargetSquare[1]){
                *leftcount = *rightcount;
                *(leftcount + 1) = *(rightcount + 1); // При совпадении происходит запись хода и дальнейший поиск
                leftcount += 2;
                rightcount += 2;
                break;
            }
        }
        
        /* Достижение циклом конца доступных ходов означает, что в нужном направлении ходов больше нет,
        остаток строки сдвигается влево к последнему ходу блокированной фигуры */ 
        if (*rightcount == '/' || *rightcount == '\0'){
            for (; *leftcount; leftcount++, rightcount++)
                *leftcount = *rightcount;
            
            return;
        }
    }
    
}

// Проверка, атакована ли клетка вражеским королем
inline int cell :: king_threat (int mode)
{
    if (Colour != Game.CurrentColour && Name == King)
        return mode;
    return 2;
}

// Проверка, атакована ли клетка вражеской ладьей или ферзем, закрытие от шаха и корректировка блокированных фигур
// Функция вызывается из цикла внутри check_king() и возвращает 0, 1 или 2.
int cell :: rook_threat (int mode, int step, king_safety& SafetyInfo)
{
    if (Colour == NoColour) // Клетка свободна, проверка продолжается
        return 2;
    
    if (Colour == Game.CurrentColour){
        if (SafetyInfo.Ally)
            return 1; // По направлению стоит больше одного союзника, проверка останавливается
        else{
            SafetyInfo.Ally = this;
            return 2; // По направлению стоит союзник, его адрес запоминается, проверка продолжается
        }
    }
    
    if (Name != Queen && Name != Rook)
        return 1; // Обнаружена безопасная вражеская фигура, проверка останавливается
    
    if (SafetyInfo.Ally != 0){ // Обнаружена опасная вражеская фигура, блокирующая союзную фигуру
        switch (mode){
            case 0: 
                return 1; // Клетка считается подходящей для перемещения короля, проверка останавливается
            case 1:
                // Блокированная фигура записывается для дальнейшей корректировки, проверка останавливается
                SafetyInfo.Ally -> writesquare(SafetyInfo.Pinned, 1);
                writestep(SafetyInfo.Pinned, step);
                writesquare(SafetyInfo.Pinned, 1);
                return 1;
        }
    }
    
    if (SafetyInfo.Ally == 0){ // Обнаружена опасная вражеская фигура, дающая шах королю
        switch (mode){
            case 0:
                return 0; //Клетка считается неподходящей для перемещения короля, проверка останавливается
            case 1:
                // Вражеская фигура добавляется к атакующим для дальнейшей корректировки, проверка останавливается
                SafetyInfo.Attackers++;
                writesquare(SafetyInfo.Threats);
                writestep(SafetyInfo.Threats, -step);
                return 1;
        }
    }
    return 1;
}

// Аналогичная предыдущей функция для слона и ферзя
int cell :: bishop_threat (int mode, int step, king_safety& SafetyInfo)
{
    if (Colour == NoColour) // Клетка свободна, проверка продолжается
        return 2;
    
    if (Colour == Game.CurrentColour){
        if (SafetyInfo.Ally)
            return 1; // По направлению стоит больше одного союзника, проверка останавливается
        else{
            SafetyInfo.Ally = this;
            return 2; // По направлению стоит союзник, его адрес запоминается, проверка продолжается
        }
    }
    
    if (Name != Queen && Name != Bishop)
        return 1; // Обнаружена безопасная вражеская фигура, проверка останавливается
    
    if (SafetyInfo.Ally != 0){ // Обнаружена опасная вражеская фигура, блокирующая союзную фигуру
        switch (mode){
            case 0: 
                return 1; // Клетка считается подходящей для перемещения короля, проверка останавливается
            case 1:
                // Блокированная фигура записывается для дальнейшей корректировки, проверка останавливается
                SafetyInfo.Ally -> writesquare(SafetyInfo.Pinned, 1);
                writestep(SafetyInfo.Pinned, step);
                writesquare(SafetyInfo.Pinned, 1);
                return 1;
        }
    }
    
    if (SafetyInfo.Ally == 0){ // Обнаружена опасная вражеская фигура, дающая шах королю
        switch (mode){
            case 0:
                return 0; //Клетка считается неподходящей для перемещения короля, проверка останавливается
            case 1:
                // Вражеская фигура добавляется к атакующим для дальнейшей корректировки, проверка останавливается
                SafetyInfo.Attackers++;
                writesquare(SafetyInfo.Threats);
                writestep(SafetyInfo.Threats, -step);
                return 1;
        }
    }
    return 1;
}

// Проверка, атакована ли клетка пешкой
int cell :: pawn_threat (int mode, int step, king_safety& SafetyInfo)
{
    if (step == StepUp + StepLeft || step == StepUp + StepRight){
        if (Colour == Black && Name == Pawn && Game.CurrentColour == White){
            switch (mode){
                case 0:
                    return 0;
                case 1:
                    writesquare (SafetyInfo.Threats);
                    strcat (SafetyInfo.Threats, "Pw"); // Пешка атакует на расстоянии одной клетки, шаг направления не нужен
                    SafetyInfo.Attackers++;
                    return 1;
            }
        }
    }
    if (step == StepDown + StepLeft || step == StepDown + StepRight){
        if (Colour == White && Name == Pawn && Game.CurrentColour == Black){
            switch (mode){
                case 0:
                    return 0;
                case 1:
                    writesquare(SafetyInfo.Threats);
                    strcat (SafetyInfo.Threats, "Pw"); // Пешка атакует на расстоянии одной клетки, шаг направления не нужен
                    SafetyInfo.Attackers++;
                    return 1;
            }
        }
    }
    return 1;
}

// Проверка, атакована ли клетка конем
int cell :: knight_threat (int mode, king_safety& SafetyInfo)
{
    if (Name != Knight)
        return 2;
    
    if (Colour != NoColour && Colour != Game.CurrentColour){
        switch (mode){
            case 0:
                return 0;
            case 1:
                writesquare(SafetyInfo.Threats);
                strcat(SafetyInfo.Threats, "Kn"); // Блокировать шах от коня невозможно, шаг направления не нужен
                SafetyInfo.Attackers++;
                return 1;
        }
    }
    return 2;
}

/* Проверка безопасности короля. При mode = 0 (по умолчанию), проверяется, атакована ли клетка со всех возможных
направлений. При mode = 1, проверка аналогична, но обрабатывается ситуация шаха и блокировки фигур */
bool cell :: check_king(int mode)
{
    int step;
    int threat_mode; // В эту переменную пишется результат проверки функций типа ..._threat()
    cell *tempSquare = 0;
    cell *SquareZero = &Board[0][0];
    cell *SquareEnd = &Board[7][7];
    char Headline[5] = "";
    char KingEscape[17] = "";
    king_safety SafetyInfo;
    char* Pinned = 0;
    
    Headline[0] = '/';
    writesquare(Headline);
    Headline[3] = ':';
    
    // Проверка по вертикали и горизонтали
    
    if (Vertical_Coord < 7){
        tempSquare = this + StepUp;
        
        if (tempSquare -> king_threat(mode) == 0)
            return false;
        
        for (step = StepUp; tempSquare -> Vertical_Coord <= 7 && tempSquare <= SquareEnd; tempSquare += step){
            
            threat_mode = tempSquare -> rook_threat(mode, step, SafetyInfo);
            if (threat_mode == 0)
                return false;
            if (threat_mode == 1)
                break;
        }
        SafetyInfo.Ally = 0;
    }
    
    if (Vertical_Coord > 0){
        tempSquare = this + StepDown;
        
        if (tempSquare -> king_threat(mode) == 0)
            return false;
        
        for (step = StepDown; tempSquare -> Vertical_Coord >= 0 && tempSquare >= SquareZero; tempSquare += step){
            threat_mode = tempSquare -> rook_threat(mode, step, SafetyInfo);
            if (threat_mode == 0)
                return false;
            if (threat_mode == 1)
                break;
        }
        SafetyInfo.Ally = 0;
    }
    
    if (Horizontal_Coord < 7){
        tempSquare = this + StepRight;
        
        if (tempSquare -> king_threat(mode) == 0)
            return false;
        
        for (step = StepRight; tempSquare <= SquareEnd; tempSquare += step){
            threat_mode = tempSquare -> rook_threat(mode, step, SafetyInfo);
            if (threat_mode == 0)
                return false;
            if (threat_mode == 1)
                break;
        }
        SafetyInfo.Ally = 0;
    }
    
    if (Horizontal_Coord > 0){
        tempSquare = this + StepLeft;
        
        if (tempSquare -> king_threat(mode) == 0)
            return false;
        
        for (step = StepLeft; tempSquare >= SquareZero; tempSquare += step){
            threat_mode = tempSquare -> rook_threat(mode, step, SafetyInfo);
            if (threat_mode == 0)
                return false;
            if (threat_mode == 1)
                break;
        }
        SafetyInfo.Ally = 0;
    }
    
    // Проверка по диагоналям
    
    if (Horizontal_Coord > 0 && Vertical_Coord < 7){
        tempSquare = this + StepUp + StepLeft;
        
        if (tempSquare -> pawn_threat(mode, step, SafetyInfo) == 0)
            return false;
        
        if (tempSquare -> king_threat(mode) == 0)
            return false;
        
        for (step = StepUp + StepLeft; tempSquare -> Vertical_Coord <= 7 && tempSquare >= SquareZero; tempSquare += step){
            threat_mode = tempSquare -> bishop_threat(mode, step, SafetyInfo);
            if (threat_mode == 0)
                return false;
            if (threat_mode == 1)
                break;
        }
        SafetyInfo.Ally = 0;
    }
    
    if (Horizontal_Coord < 7 && Vertical_Coord < 7){
        tempSquare = this + StepUp + StepRight;
        
        if (tempSquare -> pawn_threat(mode, step, SafetyInfo) == 0)
            return false;
        
        if (tempSquare -> king_threat(mode) == 0)
            return false;
        
        for (step = StepUp + StepRight; tempSquare -> Vertical_Coord <= 7 && tempSquare <= SquareEnd; tempSquare += step){
            threat_mode = tempSquare -> bishop_threat(mode, step, SafetyInfo);
            if (threat_mode == 0)
                return false;
            if (threat_mode == 1)
                break;
        }
        SafetyInfo.Ally = 0;
    }
    
    if (Horizontal_Coord > 0 && Vertical_Coord > 0){
        tempSquare = this + StepDown + StepLeft;
        
        if (tempSquare -> pawn_threat(mode, step, SafetyInfo) == 0)
            return false;
        
        if (tempSquare -> king_threat(mode) == 0)
            return false;
       
        for (step = StepDown + StepLeft; tempSquare -> Vertical_Coord <= 7 && tempSquare >= SquareZero; tempSquare += step){
            threat_mode = tempSquare -> bishop_threat(mode, step, SafetyInfo);
            if (threat_mode == 0)
                return false;
            if (threat_mode == 1)
                break;
        }
        SafetyInfo.Ally = 0;
    }
    
    if (Horizontal_Coord < 7 && Vertical_Coord > 0){
        tempSquare = this + StepDown + StepRight;
        
        if (tempSquare -> pawn_threat(mode, step, SafetyInfo) == 0)
            return false;
        
        if (tempSquare -> king_threat(mode) == 0)
            return false;
       
        for (step = StepDown + StepRight; tempSquare -> Vertical_Coord <= 7 && tempSquare >= SquareZero; tempSquare += step){
            threat_mode = tempSquare -> bishop_threat(mode, step, SafetyInfo);
            if (threat_mode == 0)
                return false;
            if (threat_mode == 1)
                break;
        }
        SafetyInfo.Ally = 0;
    }
    
    // Проверка коней
    
    if (Vertical_Coord > 0 && Horizontal_Coord > 1){
        tempSquare = this + 2*StepLeft + StepDown;
        
        if (tempSquare -> knight_threat(mode, SafetyInfo) == 0)
            return false;
    }
    
    if (Vertical_Coord > 0 && Horizontal_Coord < 6){
        tempSquare = this + 2*StepRight + StepDown;
        
        if (tempSquare -> knight_threat(mode, SafetyInfo) == 0)
            return false;
    }
            
    if (Vertical_Coord < 7 && Horizontal_Coord > 1){
        tempSquare = this + 2*StepLeft + StepUp;
        
        if (tempSquare -> knight_threat(mode, SafetyInfo) == 0)
            return false;
    }
            
    if (Vertical_Coord < 7 && Horizontal_Coord < 6){
        tempSquare = this + 2*StepRight + StepUp;
        
        if (tempSquare -> knight_threat(mode, SafetyInfo) == 0)
            return false;
    }
            
    if (Vertical_Coord > 1 && Horizontal_Coord > 0){ 
        tempSquare = this + StepLeft + 2*StepDown;
        
        if (tempSquare -> knight_threat(mode, SafetyInfo) == 0)
            return false;
    }
            
    if (Vertical_Coord > 1 && Horizontal_Coord < 7){ 
        tempSquare = this + StepRight + 2*StepDown;
        
        if (tempSquare -> knight_threat(mode, SafetyInfo) == 0)
            return false;
    }
            
    if (Vertical_Coord < 6 && Horizontal_Coord > 0){ 
        tempSquare = this + StepLeft + 2*StepUp;
        
        if (tempSquare -> knight_threat(mode, SafetyInfo) == 0)
            return false;
    }
            
    if (Vertical_Coord < 6 && Horizontal_Coord < 7){ 
        tempSquare = this + StepRight + 2*StepUp;
        
        if (tempSquare -> knight_threat(mode, SafetyInfo) == 0)
            return false;
    }
    
    // По завершению проверки происходит корректировка Game.CorrectMoves
    switch (mode){
        case 0:
            return true;
        case 1:
            if (SafetyInfo.Attackers > 1){ 
                // При атаке двумя фигурами блокировка шаха невозможна, остаются только ходы короля
                for (char* p = Game.CorrectMoves; *p; p++)
                    *p = '\0';
                
                king_moves (KingEscape);
                
                if (strcmp(KingEscape, "")){
                    strcat(Game.CorrectMoves, Headline);
                    strcat(Game.CorrectMoves, KingEscape);
                }
                return false;
            }
            
            if (SafetyInfo.Attackers == 1){
                // Обработка шаха от одной фигуры
                SafetyInfo.Threats[4] = 'a' + char(Horizontal_Coord);
                SafetyInfo.Threats[5] = '1' + char(Vertical_Coord);
                threat_block (SafetyInfo.Threats); // Блокировка шаха
                king_moves (KingEscape); // Добавление ходов короля
                
                if (strcmp(KingEscape, "")){
                    strcat(Game.CorrectMoves, Headline);
                    strcat(Game.CorrectMoves, KingEscape);
                }
                
                if (strcmp(Game.CorrectMoves, "") && strcmp(SafetyInfo.Pinned, "")){
                    Pinned = SafetyInfo.Pinned;
                    while (*Pinned){
                        pinned_exclude (Pinned); // Исключение блокированных фигур
                        Pinned += 6;
                    }
                }
                
                return false;
            }
            
            // Шаха нет, но производится исключение блокированных фигур
            if (strcmp(SafetyInfo.Pinned, "")){
                Pinned = SafetyInfo.Pinned;
                    while (*Pinned){
                        pinned_exclude (Pinned);
                        Pinned += 6;
                    }
                return true;
            }
    }
    return true;
}

// Расчет всех возможных ходов для всех клеток
bool count_moves ()
{
    bool NoCheck = true;
    
    // Проверка фигур осуществляется из массива Game.PiecePointer, начиная с индекса, соответствующего цвету (0 / 16)
    int count = (int) Game.CurrentColour;
    
    cell* Piece = 0;
    
    for (char *p = Game.CorrectMoves; *p; p++) // Очистка строки
        *p = '\0';
    
    for (int i = 0; i < 16; i++, count++){
        Piece = Game.PiecePointer[count];
        if (Piece)
            Piece -> movement_list(); // Если фигура есть на доске, для нее рассчитываются и пишутся все ходы
    }
    
    NoCheck = Piece -> check_king(1); // Полная проверка текущего положения короля
    
    if (!strcmp(Game.CorrectMoves, "") && NoCheck){
        cout << "Пат, ничья, игра окончена";
        return false;
    }
    
    if (!strcmp(Game.CorrectMoves, "") && !NoCheck){
        cout << "Шах и мат, игра окончена";
        return false;
    }
    
    if (NoCheck)
        check_castle();
    
    //cout << Game.CorrectMoves << '\n';
    return true;
}

// Проверка введенной команды на правильность
bool read_command (char* command)
{   
    char *count;
    int h1, h2, v1, v2;
    
    if (strlen(command) == 4){
        if (command[0] < 'a' || command[0] > 'h') return false;
        if (command[1] < '1' || command[1] > '8') return false;
        if (command[0] < 'a' || command[0] > 'h') return false;
        if (command[3] < '1' || command[3] > '8') return false;
    }
    
    if (strlen(command) == 3 && strcmp(command, "O-O")) return false;
    
    if (strlen(command) == 5 && strcmp(command, "O-O-O")) return false;
    
    // Поиск в Game.CorrectMoves хода O-O
    if (!strcmp(command, "O-O")){
        for (count = Game.CorrectMoves; *count; count++)
            if (*count == 'O')
                if (*(count + 1) == '-' && *(count + 2) == 'O' && *(count + 3) != '-'){
                    short_castle();
                    return true;
                }
    }
    
    // Поиск в Game.CorrectMoves хода O-O-O
    if (!strcmp(command, "O-O-O")){
        for (count = Game.CorrectMoves; *count; count++)
            if (*count == 'O')
                if (*(count + 1) == '-', *(count + 2) == 'O', *(count + 3) == '-', *(count + 4) == 'O'){
                    long_castle();
                    return true;
                }
    }
    
    // Поиск в Game.CorrectMoves исходной клетки введенного хода
    for (count = Game.CorrectMoves; *(count + 1); count++){
        if (*count == '/'){
            if (*(count + 1) == command[0] && *(count + 2) == command[1]){
                count += 4;
                break;
            }
        }
    }
    
    if (!*(count + 1)){
        cout << '\n' << "Неправильный ход" << '\n';
        return false;
    }
    
    // Поиск в Game.CorrectMoves конечной введенной клетки
    for (; *count != '/'; count += 2){
        if (*count == command[2] && *(count + 1) == command[3]){
            h2 = int(command[0]) - int('a');
            v2 = int(command[1]) - int('1');
            h1 = int(command[2]) - int('a');
            v1 = int(command[3]) - int('1');
            Board[h1][v1] = Board[h2][v2];
            return true;
        }
    }
    cout << '\n' << "Неправильный ход" << '\n';
    return false;
}

int main()
{
    char command[6];
    
    init_board();
    load_FEN (startFEN);
    show_board();
    
    while (count_moves()){
        
        do {
            cout << "Ваш ход:";
            cin >> command;
        }while (!read_command (command));
        
        if (Game.CurrentColour == White)
            Game.CurrentColour = Black;
        else
            Game.CurrentColour = White;
        
        show_board();
    }
    
    return 0;
}
