#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

const int gridSize = 22;  // 格子大小
int width = 30;           // 游戏区域的宽度，单位为格子
int height = 16;          // 游戏区域的高度，单位为格子
int mineCount = 99;       // 雷的数量

enum CellState { CLOSED, OPENED, FLAGGED };

// 写一个函数判断一个字符串是否由两个空格分开了三个数字
bool is_valid(std::string input) {
    int space = 0;
    for (auto c : input) {
        if (c == ' ') space++;
    }

    if (space != 2) return false;

    std::stringstream ss(input);
    int w = 0, h = 0, m = 0;
    ss >> w >> h >> m;

    if (w && h && m) return true;
    return false;
}

struct Cell {
    bool mine;
    int adjacentMines;
    CellState state;

    Cell() : mine(false), adjacentMines(0), state(CLOSED) {}
};

class Minesweeper : public Fl_Window {
   private:
    std::vector<std::vector<Cell>> grid;

    bool Gamestarted = false;
    bool Gameover = false;
    bool Levelsetted = false;
    int minesLeft = mineCount;

    // 添加一个重置按钮
    Fl_Button* resetbutton;

    // 添加一个输出剩余雷数的文本框
    Fl_Box* mineCountBox;

    // 添加一个计时器文本框
    Fl_Output* timerBox;

    // 添加四个选择难度的按钮
    Fl_Button* easyButton;
    Fl_Button* mediumButton;
    Fl_Button* hardButton;
    Fl_Button* Diybutton;

   public:
    // 计时器
    static int seconds;

    Minesweeper(int w, int h, const char* title)
        : Fl_Window(w * gridSize, h * gridSize + 50, title),
          Gamestarted(false) {
        // 初始化难度选择按钮
        easyButton =
            new Fl_Button(w * gridSize / 3, h * gridSize / 5, 220, 50, "Easy");
        easyButton->callback(easy_cb, this);

        mediumButton = new Fl_Button(w * gridSize / 3, h * gridSize * 2 / 5,
                                     220, 50, "Medium");
        mediumButton->callback(medium_cb, this);

        hardButton = new Fl_Button(w * gridSize / 3, h * gridSize * 3 / 5, 220,
                                   50, "Hard");
        hardButton->callback(hard_cb, this);

        Diybutton = new Fl_Button(w * gridSize / 3, h * gridSize * 4 / 5, 220,
                                  50, "DIY");
        Diybutton->callback(Diy_cb, this);

        // 初始化游戏区域
        grid.resize(height, std::vector<Cell>(width));
        for (auto& row : grid)
            for (auto& cell : row) cell = Cell();

        minesLeft = mineCount;

        // 初始化重置按钮
        resetbutton =
            new Fl_Button(10, h * gridSize + 10, 100, 30, "Play Again");
        resetbutton->callback(reset_cb, this);
        resetbutton->hide();

        // 初始化剩余雷数文本框
        mineCountBox =
            new Fl_Box(w * gridSize - 120, h * gridSize + 10, 100, 30);
        mineCountBox->label(std::to_string(minesLeft).c_str());
        mineCountBox->box(FL_BORDER_BOX);
        mineCountBox->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);

        // 初始化计时器文本框
        timerBox = new Fl_Output(300, h * gridSize + 10, 100, 30);
        timerBox->value("00:00");
    }

    static void reset_cb(Fl_Widget* widget, void* data) {
        // 重置游戏
        Minesweeper* game = static_cast<Minesweeper*>(data);
        game->reset();
    }

    static void easy_cb(Fl_Widget* widget, void* data) {
        // 设置游戏难度为简单
        Minesweeper* game = static_cast<Minesweeper*>(data);

        if (!game->Levelsetted) {
            game->set_easy();
            game->Levelsetted = true;
        }
    }

    void set_easy() {
        // 设置游戏难度为简单
        width = 9;
        height = 9;
        mineCount = 10;
        initialize();
        redraw();
    }

    static void medium_cb(Fl_Widget* widget, void* data) {
        // 设置游戏难度为中等
        Minesweeper* game = static_cast<Minesweeper*>(data);

        if (!game->Levelsetted) {
            game->set_medium();
            game->Levelsetted = true;
        }
    }

    void set_medium() {
        // 设置游戏难度为中等
        width = 16;
        height = 16;
        mineCount = 40;
        initialize();
        redraw();
    }

    static void hard_cb(Fl_Widget* widget, void* data) {
        // 设置游戏难度为困难
        Minesweeper* game = static_cast<Minesweeper*>(data);

        if (!game->Levelsetted) {
            game->set_hard();
            game->Levelsetted = true;
        }
    }

    void set_hard() {
        // 设置游戏难度为困难
        width = 30;
        height = 16;
        mineCount = 99;
        initialize();
        redraw();
    }

    static void Diy_cb(Fl_Widget* widget, void* data) {
        // 设置游戏难度为自定义
        Minesweeper* game = static_cast<Minesweeper*>(data);

        if (!game->Levelsetted) {
            const char* input;
            input = fl_input(
                "Please input the numbers of the size of map and "
                "the mines you want to set:");

            if (input == nullptr) {
                game->reset();
                return;
            }

            std::string inpu(input);

            // 去掉inpu第一个数字前的空格
            while (inpu[0] == ' ') {
                inpu.erase(0, 1);
            }

            // 如果选择取消，返回
            if (inpu.empty()) {
                fl_alert("Please input three numbers!");
                game->reset();
                return;
            }

            // 检查输入是否合法
            if (!is_valid(inpu)) {
                fl_alert("Please input three numbers!");
                game->reset();
                return;
            }

            // 将以空格分开的三个数字分别存入w,h,m
            int w = 0, h = 0, m = 0;
            std::stringstream ss(input);
            ss >> w >> h >> m;

            if (w && h && m) {
                if (mineCount > width * height) {
                    fl_alert("The number of mines is too large!");
                    game->reset();
                }

                else {
                    game->set_diy(w, h, m);
                    game->Levelsetted = true;
                }
            }

            else {
                fl_alert("Please input three numbers!");
                game->reset();
            }
        }
    }

    void set_diy(int w, int h, int m) {
        // 设置游戏难度为自定义
        width = w;
        height = h;
        mineCount = m;
        initialize();
        redraw();
    }

    void placeMines(int r, int c) {
        // 随机放置雷
        srand((unsigned)time(nullptr));
        int minesPlaced = 0;

        while (minesPlaced < mineCount) {
            int row = rand() % height;
            int column = rand() % width;

            // 跳过第一次点击的格子
            if (row == r && column == c) continue;
            if (!grid[row][column].mine) {
                grid[row][column].mine = true;
                minesPlaced++;
            }
        }
    }

    void calculateAdjacentMines() {
        // 计算每个格子周围的雷数
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                int adjacentMines = 0;
                for (int dr = -1; dr <= 1; ++dr) {
                    for (int dc = -1; dc <= 1; ++dc) {
                        int nr = r + dr;
                        int nc = c + dc;

                        if (nr >= 0 && nr < height && nc >= 0 && nc < width &&
                            grid[nr][nc].mine) {
                            adjacentMines++;
                        }
                    }
                }
                grid[r][c].adjacentMines = adjacentMines;
            }
        }
    }

    int handle(int event) override {
        // 处理鼠标点击事件
        if ((!Gameover) && Levelsetted) {
            if (event == FL_PUSH) {
                int r = Fl::event_y() / gridSize;
                int c = Fl::event_x() / gridSize;

                if (Fl::event_button() == FL_LEFT_MOUSE) {
                    // 处理左键点击
                    openCell(r, c);
                }

                else if (Fl::event_button() == FL_RIGHT_MOUSE) {
                    // 处理右键点击
                    flagCell(r, c);
                }

                return 1;
            }

            else
                return Fl_Window::handle(event);
        }

        else
            return Fl_Window::handle(event);
    }

    void openCell(int r, int c) {
        // 打开单元格

        if (r < 0 || r >= height || c < 0 || c >= width) return;
        // 越界检查

        if (!Gamestarted) {
            Gamestarted = true;

            // 开始计时
            Fl::add_timeout(1.0, update_timer, timerBox);

            // 放置雷并计算邻近雷数
            placeMines(r, c);
            calculateAdjacentMines();
        }

        if (grid[r][c].state == CLOSED) {
            std::queue<std::pair<int, int>> toOpen;
            toOpen.push({r, c});

            while (!toOpen.empty()) {
                std::pair<int, int> current = toOpen.front();
                toOpen.pop();
                int currentRow = current.first;
                int currentColumn = current.second;
                Cell& cell = grid[currentRow][currentColumn];

                if (cell.state != CLOSED) continue;
                // 跳过非封闭格子

                cell.state = OPENED;
                // 打开格子

                if (cell.mine) {
                    // 触雷，游戏结束
                    endgame(false);
                    // 实际游戏中应该还有结束游戏的逻辑
                    break;
                }

                else if (cell.adjacentMines == 0) {
                    // 没有邻近雷，将周围的格子加入队列
                    for (int dr = -1; dr <= 1; ++dr) {
                        for (int dc = -1; dc <= 1; ++dc) {
                            int nr = currentRow + dr;
                            int nc = currentColumn + dc;
                            // 越界检查和跳过自身
                            if (nr < 0 || nr >= height || nc < 0 ||
                                nc >= width || (dr == 0 && dc == 0))
                                continue;
                            // 只有闭合的格子才加入队列
                            if (grid[nr][nc].state == CLOSED) {
                                toOpen.push(std::make_pair(nr, nc));
                            }
                        }
                    }
                }
            }

            if (checkVictory()) {
                // 检查是否获胜
                endgame(true);
                // 实际游戏中应该还有结束游戏的逻辑
                Gamestarted = false;
                Gameover = true;
            }
            redraw();  // 重绘游戏区域
        }
    }

    void flagCell(int r, int c) {
        // 标记或取消标记单元格
        if (r < 0 || r >= height || c < 0 || c >= width) return;
        // 越界检查

        if (!Gamestarted) return;
        // 游戏未开始

        if (grid[r][c].state == OPENED && grid[r][c].adjacentMines == 0) return;
        // 跳过已经打开的格子

        if (grid[r][c].state == CLOSED) {
            grid[r][c].state = FLAGGED;
            minesLeft--;
        }

        else if (grid[r][c].state == FLAGGED) {
            grid[r][c].state = CLOSED;
            minesLeft++;
        }

        // 右键点击非地雷按钮，若周围地雷都已经被标记，则自动打开周围未打开的单元格
        if (grid[r][c].state == OPENED && grid[r][c].adjacentMines > 0) {
            int flaggedMines = 0;
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    int nr = r + dr;
                    int nc = c + dc;
                    if (nr >= 0 && nr < height && nc >= 0 && nc < width &&
                        grid[nr][nc].state == FLAGGED) {
                        flaggedMines++;
                    }
                }
            }

            if (flaggedMines == grid[r][c].adjacentMines) {
                for (int dr = -1; dr <= 1; ++dr) {
                    for (int dc = -1; dc <= 1; ++dc) {
                        int nr = r + dr;
                        int nc = c + dc;
                        if (nr >= 0 && nr < height && nc >= 0 && nc < width &&
                            grid[nr][nc].state == CLOSED) {
                            openCell(nr, nc);
                        }
                    }
                }
            }
        }

        if (checkVictory()) {
            endgame(true);
            Gamestarted = false;
            Gameover = true;
        }

        redraw();
    }

    bool checkVictory() const {
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                const Cell& cell = grid[r][c];
                // 如果有一个没有雷的格子还没被打开，玩家尚未获胜
                if (!cell.mine && cell.state != OPENED) {
                    return false;
                }
            }
        }

        // 如果所有没有雷的格子都被打开，玩家获胜
        return true;
    }

    static void update_timer(void* data) {
        // 将时间转换为分钟:秒格式
        int minutes = seconds / 60;
        int remainder_seconds = seconds % 60;

        // 更新时间显示
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << minutes << ":"
           << std::setw(2) << std::setfill('0') << remainder_seconds;
        ((Fl_Output*)data)->value(ss.str().c_str());

        ++seconds;

        // 每秒调用一次update_timer函数
        Fl::repeat_timeout(1.0, update_timer, data);
    }

    void reset() {
        // 重置游戏
        Gamestarted = false;
        Gameover = false;
        Levelsetted = false;
        // initialize();
        redraw();
    }

    void initialize() {
        // 初始化游戏
        grid.clear();
        grid.resize(height, std::vector<Cell>(width));
        for (auto& row : grid)
            for (auto& cell : row) cell = Cell();
        resetbutton->hide();
        timerBox->value("00:00");

        minesLeft = mineCount;
        redraw();
    }

    void endgame(bool won) {
        // 游戏结束
        if (!Gameover) {
            // 停止计时
            Fl::remove_timeout(update_timer);

            if (won) {
                // 显示获胜信息
                fl_alert("Congratulations! You win! You used only %d seconds!",
                         seconds);
            }

            else {
                // 显示失败信息
                fl_alert("Boom! You hit a mine!");
            }

            Gameover = true;
            // 显示重置按钮
            resetbutton->show();

            // 重置计时器
            seconds = 0;
            redraw();
        }
    }

    void draw() override {
        if (Levelsetted) {
            easyButton->hide();
            mediumButton->hide();
            hardButton->hide();
            Diybutton->hide();

            // 绘制剩余雷数
            std::stringstream ss;
            ss << "Mines left : " << minesLeft;
            mineCountBox->label(ss.str().c_str());
            Fl_Window::draw();

            //  绘制游戏界面
            if (Levelsetted && !Gameover) {
                // 隐藏重置按钮
                resetbutton->hide();

                // 绘制格子
                for (int r = 0; r < height; ++r) {
                    for (int c = 0; c < width; ++c) {
                        int x = c * gridSize;
                        int y = r * gridSize;

                        if (grid[r][c].state == OPENED) {
                            fl_draw_box(FL_FLAT_BOX, x, y, gridSize, gridSize,
                                        FL_BLUE);

                            if (grid[r][c].mine) {
                                // 绘制雷
                                fl_color(FL_BLACK);
                                fl_pie(x + 2, y + 2, gridSize - 4, gridSize - 4,
                                       0.0, 360.0);
                            }

                            else if (grid[r][c].adjacentMines > 0) {
                                // 绘制邻近雷数
                                fl_color(FL_WHITE);
                                fl_font(FL_HELVETICA, 16);
                                fl_draw(std::to_string(grid[r][c].adjacentMines)
                                            .c_str(),
                                        x + 6, y + 14);
                            }
                        }

                        else {
                            fl_draw_box(FL_UP_BOX, x, y, gridSize, gridSize,
                                        FL_GRAY);
                            if (grid[r][c].state == FLAGGED) {
                                // 绘制旗帜
                                fl_color(FL_RED);
                                fl_pie(x + 2, y + 2, gridSize - 4, gridSize - 4,
                                       0.0, 360.0);
                            }
                        }
                    }
                }
            }

            else {
                resetbutton->hide();
                for (int r = 0; r < height; ++r) {
                    for (int c = 0; c < width; ++c) {
                        int x = c * gridSize;
                        int y = r * gridSize;
                        fl_draw_box(FL_UP_BOX, x, y, gridSize, gridSize,
                                    FL_GRAY);
                    }
                }
            }

            if (Gameover) {
                // 绘制所有雷
                fl_color(FL_BLACK);
                for (int r = 0; r < height; ++r) {
                    for (int c = 0; c < width; ++c) {
                        int x = c * gridSize;
                        int y = r * gridSize;
                        if (grid[r][c].mine) {
                            fl_pie(x + 2, y + 2, gridSize - 4, gridSize - 4,
                                   0.0, 360.0);
                        }
                    }
                }

                // 绘制错误标记的雷
                fl_color(FL_RED);
                for (int r = 0; r < height; ++r) {
                    for (int c = 0; c < width; ++c) {
                        int x = c * gridSize;
                        int y = r * gridSize;
                        if (grid[r][c].state == FLAGGED && !grid[r][c].mine) {
                            fl_pie(x + 2, y + 2, gridSize - 4, gridSize - 4,
                                   0.0, 360.0);
                        }
                    }
                }

                // 绘制重置按钮
                resetbutton->show();
            }
        }

        else {
            easyButton->show();
            mediumButton->show();
            hardButton->show();
            Diybutton->show();
            resetbutton->hide();
            timerBox->value("00:00");
            mineCountBox->label("Mines left : ");
            Fl_Window::draw();
        }
    }
};

int Minesweeper::seconds = 0;

int main() {
    Minesweeper mine(width, height, "Minesweeper");
    mine.show();
    return Fl::run();
}
