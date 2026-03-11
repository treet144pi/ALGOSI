#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <climits>

//перевод символа в цифру
int ToDigit(char ch)
{
    if (ch == '#') return 2;
    if (ch == '?') return 1;
    return 0;               // '-'
}

// нужна чтобы оставить слова которые могли бы дать такой же фидбек как и загаданное
std::uint16_t FeedbackIdCount(const std::string&      guess,
                              const std::string&      secret,
                              int                     L,
                              const std::vector<int>& pow3)
{
    int cnt[26] = {0};
    int d[16]   = {0};

    for (int i = 0; i < L; ++i) {
        char g = guess[i];
        char s = secret[i];
        if (g == s) d[i] = 2;
        else        cnt[s - 'a']++;
    }

    for (int i = 0; i < L; ++i) {
        if (d[i] == 0)
        {
            int gi = guess[i] - 'a';
            if (cnt[gi] > 0)
            {
                d[i] = 1;
                cnt[gi]--;
            }
        }
    }

    int id = 0;
    //go to 3 с.ч
    for (int i = 0; i < L; ++i) id += d[i] * pow3[i];

    return static_cast<std::uint16_t>(id);
}

// нужна для поиска лучшего слова
int ScoreByLetterFrequencie(const std::string& w,
                            const std::vector<std::vector<int>>& posCnt,
                            const int presCnt[26],
                            int L)
{
    bool used[26] = {false};
    int sc = 0;
    for (int i = 0; i < L; ++i)
    {
        int c = w[i] - 'a';
        sc += posCnt[i][c];
        if (!used[c]) { used[c] = true; sc += presCnt[c]; }
    }
    return sc;
}

int ChooseNextGuess(int N, int A, int L, int          numPatterns,
                    const std::vector<std::string>&   dict,
                    const std::vector<std::uint16_t>& fb,
                    const std::vector<int>&           possible)
{
    const int S = possible.size();
    if (S == 1) return possible[0];


    //количество слов которые имеют ту или иную букву на определнной позиции
    std::vector<std::vector<int>> posCnt(L, std::vector<int>(26, 0));

    // в скольких словах есть та или иная букава
    int presCnt[26] = {0};


    // заполняем эти два массив с возможномы ответами
    for (int sec : possible) {
        bool used[26] = {false};
        const std::string& w = dict[sec];

        for (int i = 0; i < L; ++i) {
            int c = w[i] - 'a';
            posCnt[i][c]++;
            if (!used[c]) { used[c] = true; presCnt[c]++; }
        }
    }


    int K; // количество слов которые мы хотим оставить
    if (S > 1200)     K = 140;
    else if (S > 600) K = 200;
    else              K = 260;

    std::vector<std::pair<int,int>> heap;
    heap.reserve(K);

    //for min heap
    auto heapCmp = [](const std::pair<int,int>& a, const std::pair<int,int>& b){
        return a.first > b.first;
    };

    //THE BEST K words
    for (int i = 0; i < N; ++i) {
        int sc = ScoreByLetterFrequencie(dict[i], posCnt, presCnt, L);
        if (static_cast<int>(heap.size()) < K)
        {
            heap.push_back({sc, i}); // добавляем в кучу
            if (static_cast<int>(heap.size()) == K)
                std::make_heap(heap.begin(), heap.end(), heapCmp);
        }
        else if (sc > heap.front().first)
        {
            std::pop_heap(heap.begin(), heap.end(), heapCmp);
            heap.back() = {sc, i};
            std::push_heap(heap.begin(), heap.end(), heapCmp);
        }
    }

    //minimax
    std::vector<int> cnt(numPatterns, 0); //divide our dict to group
    std::vector<int> seen(numPatterns, 0);
    int stamp = 1;
    int bestMax = INT_MAX;
    long long bestSumSq = (1LL<<62);
    int bestidx = heap.empty() ? possible[0] : heap[0].second; //idx лучшего слова

    for (const auto& pr : heap)
    {
        int g = pr.second;
        const std::size_t base = g * A;
        int mx = 0;
        long long sumsq = 0;
        stamp++;

        for (int sec : possible) {
            int id = (int)fb[base + (std::size_t)sec];

            if (seen[id] != stamp) { seen[id] = stamp; cnt[id] = 0; }
            int c = cnt[id];
            cnt[id] = c + 1;
            sumsq += 2LL * c + 1;
            if (c + 1 > mx) {
                mx = c + 1;
                if (mx > bestMax) break;
            }
        }
        if (mx > bestMax) continue;
        if (mx < bestMax || (mx == bestMax && sumsq < bestSumSq))
        {
            bestMax = mx;
            bestSumSq = sumsq;
            bestidx = g;
            if (bestMax == 1) break;
        }
    }

    return bestidx;
}

int main() {

    int N, M, L;
    std::cin >> N >> M >> L;

    std::vector<std::string> dict(N);
    for (int i = 0; i < N; ++i)
        std::cin >> dict[i]; // заполняем словарь

    const int A = std::min(2100, N); // ответ из 2100 первых слов


    // будем ответы кодировать в число в 3 системе
    std::vector<int> pow3(L + 1, 1);
    for (int i = 1; i <= L; ++i) pow3[i] = pow3[i - 1] * 3;

    const int numPatterns = pow3[L];

    // массив фидбеков
    std::vector<std::uint16_t> fb(N *A);

    for (int g = 0; g < N; ++g) {
        for (int s = 0; s < A; ++s)
        {
            fb[g *A + s] = FeedbackIdCount(dict[g], dict[s], L, pow3);
        }
    }

    // перевод ответа в число
    auto JudgePatternId = [&](const std::string& res) {
            int id = 0;
            for (int i = 0; i < L; ++i) id += ToDigit(res[i]) * pow3[i];
            return id;
    };

    while (M--) {

        std::vector<int> possible(A); //maybe possible answers in условие сказано
        for (int i = 0; i < A; ++i) possible[i] = i;


        for (int step = 0; step < 6; ++step)
        {
            int guessIdx;
            if (possible.size() == 1)  guessIdx = possible[0];
            else  guessIdx = ChooseNextGuess(N, A, L, numPatterns, dict, fb, possible); //лучшее слово


            std::cout << dict[guessIdx] << std::endl;
            std::string res;
            std::cin >> res;
            if (res == std::string(L, '#')) break; // we find our word
            int need = JudgePatternId(res);
            // to number conversion to the ternary number system

            //filter
            std::vector<int> nxt;
            nxt.reserve(possible.size());
            const size_t base = guessIdx * A;
            for (int sec : possible) {
                if (fb[base + sec] == need) nxt.push_back(sec);
            }
            possible.swap(nxt);

        }
    }

    return 0;
}
