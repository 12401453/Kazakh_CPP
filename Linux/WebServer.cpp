/* type '-lsqlite3' when compiling with g++ to include the sqlite3 library because the header file seems to just be forward declarations */
/* ICU library if installed on the system can be included with g++ <source files> `pkg-config --libs --cflags icu-uc icu-io` -o <binary name>*/
#include <sstream>
#include <fstream>
#include "WebServer.h"
#include <math.h>
#include <sys/stat.h>
#include <vector>

void WebServer::onClientConnected(int clientSocket) {

}

void  WebServer::onClientDisconnected(int clientSocket) { 
    
}

void WebServer::onMessageReceived(int clientSocket, const char* msg, int length) {


    #define GET 3
    #define POST 4
  
    m_POST_continue = true;

    printf("\n--------------------------------------MESSAGE BEGIN--------------------------------------------------\n\n");
    printf("%s", msg);
    printf("\n-----------------------------------------------------------------------------------------------------\n");
    printf("Size of the above message is %i bytes\n\n", length);
  
    int lb_pos = checkHeaderEnd(msg);
    int get_or_post = getRequestType(msg);
  
    std::cout << get_or_post << std::endl;

    if(get_or_post == -1 && lb_pos == -1) {
        buildPOSTedData(msg, false, length);

        if(m_POST_continue == false) {
            handlePOSTedData(m_post_data, clientSocket);
        }
    }

    else if (get_or_post == GET)
    {
        std::cout << "This is a GET request" << std::endl;

        std::cout << "lb_pos: " << lb_pos << std::endl;
        char msg_url[lb_pos - 12];
        std::cout << "size of msg_url array: " << sizeof(msg_url) << std::endl;

        for (int i = 4; i < lb_pos - 9; i++)
        {
            msg_url[i - 4] = msg[i];
            std::cout << msg[i];
        }
        std::cout << std::endl;
        msg_url[lb_pos - 13] = '\0';

        short int page_type = 0;
        if(!strcmp(msg_url, "/text_viewer")) page_type = 1;
        else if(!strcmp(msg_url, "/add_texts")) page_type = 2;

        //#include "docRoot.cpp"
   
        int url_size = strlen("HTML_DOCS") + sizeof(msg_url); //sizeof() can be used for c-strings declared as an array of char's but strlen() must be used for char *pointers
        char url_c_str[url_size];
    
        strcpy(url_c_str, "HTML_DOCS");
        strcat(url_c_str, msg_url);

        bool cookies_present = setCookie(m_cookie, msg);
        std::cout << "Cookies present? " << cookies_present << std::endl;
        if(cookies_present) {
            std::cout << "Cookie key: " << m_cookie[0] << "; Cookie value: " << m_cookie[1] << std::endl;
        }

        std::string content = "<h1>404 Not Found</h1>"; 

        std::string content_type = "text/html";
        
        char fil_ext[5];
        for(int i = 0; i < 4; i++) {
            fil_ext[i] = url_c_str[url_size - 5 + i];
        }
        fil_ext[4] = '\0';
        printf("fil_ext = %s\n", fil_ext);

        if(!strcmp(fil_ext, ".css")) {
            content_type = "text/css";
        }
        if(!strcmp(fil_ext + 1, ".js")) {
            content_type = "application/javascript";
        }
        if(!strcmp(fil_ext, ".ttf")) {
            content_type = "font/ttf";
            sendFontFile(url_c_str, clientSocket);
            return;
        }
       


        buildGETContent(page_type, url_c_str, content, cookies_present);       

     

        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\n";
    //    oss << "Cache-Control: no-cache, private \r\n";
        oss << "Content-Type: " << content_type << "\r\n";
        oss << "Content-Length: " << content.size() << "\r\n";
        oss << "\r\n";
        oss << content;

        std::string output = oss.str();
        int size = output.size() + 1;

        sendToClient(clientSocket, output.c_str(), size);
    }
    else if(get_or_post == POST) {  

        std::cout << "This is a POST request" << std::endl;
        
       buildPOSTedData(msg, true, length);
       if(m_POST_continue == false) {
            handlePOSTedData(m_post_data, clientSocket);
       } 
    } 
    
}


bool WebServer::c_strStartsWith(const char *str1, const char *str2)
{
    int strcmp_count = 0;
    int str2_len = strlen(str2);
 
    int i = -1;
   
    while ((*str1 == *str2 || *str2 == '\0') && i < str2_len)
    {
        strcmp_count++;
        str1++;
        str2++;
        i++;
    }
 
    if (strcmp_count == str2_len + 1)
    {
        return true;
    }
    else
        return false;
}

int WebServer::c_strFind(const char* haystack, const char* needle) {

    int needle_startpos = 0;
    int needle_length = strlen(needle);
    int haystack_length = strlen(haystack);
    if(haystack_length < needle_length) return -1;
    char needle_buf[needle_length + 1]; //yes I'm stack-allocating variable-length arrays because g++ lets me and I want the efficiency; it will segfault just the same if I pre-allocate an arbitrary length array which the data is too big for anyway, and I absolutely will not use any of the heap-allocated C++ containers for the essential HTTP message parsing, which needs to be imperceptibly fast

    needle_buf[needle_length] = '\0';
    for(int i = 0; i < haystack_length; i++) {
        for(int j = 0 ; j < needle_length; j++) {
            needle_buf[j] = haystack[j];
        }
       
        if(c_strStartsWith(needle_buf, needle)) {
            break;
        }
        needle_startpos++;
        haystack++;
    }
    
    if(needle_startpos == haystack_length) {
        return -1;
        }
    else {
        return needle_startpos;
    } 
}

int WebServer::c_strFindNearest(const char* haystack, const char* needle1, const char* needle2) {
    int needles_startpos = 0;
    int needle1_length = strlen(needle1);
    int needle2_length = strlen(needle2);
    int longest_needle_length = needle1_length < needle2_length ? needle2_length : needle1_length;
    int haystack_length = strlen(haystack);
    if(haystack_length < longest_needle_length) return -1;

    char needles_buf[longest_needle_length + 1];
    needles_buf[longest_needle_length] = '\0';

    for(int i = 0; i < haystack_length; i++) {
        for(int j = 0; j < longest_needle_length; j++) {
            needles_buf[j] = haystack[j];
        }
        if(c_strStartsWith(needles_buf, needle1) || c_strStartsWith(needles_buf, needle2)) {
            break;
        }
        needles_startpos++;
        haystack++;
    }

    if(needles_startpos == haystack_length) {
        return -1;
    }
    else {
        return needles_startpos;
    }
}


int WebServer::getRequestType(const char* msg ) {

    if(c_strStartsWith(msg, "GET")) {
        return 3;
    }
    else if(c_strStartsWith(msg, "POST")) {
        return 4;
    }
    else return -1;
}

int WebServer::checkHeaderEnd(const char* msg) {
    int lb_pos = c_strFind(msg, "\x0d");
    int HTTP_line_pos = c_strFind(msg, "HTTP/1.1");
    if(HTTP_line_pos != -1 && HTTP_line_pos < lb_pos) return lb_pos;
    else return -1;
}

 void WebServer::buildGETContent(short int page_type, char* url_c_str, std::string &content, bool cookies_present) {
    
    std::ifstream urlFile;
    urlFile.open(url_c_str);
    
    if (urlFile.good())
    {
        std::cout << "This file exists and was opened successfully." << std::endl;

        std::ostringstream ss_text;
        std::string line;
        while (std::getline(urlFile, line))
        {   
            if(page_type > 0 && line.find("<?php") != -1) insertTextSelect(ss_text);
            else if(page_type == 1 && cookies_present && line.find("<?cky") != -1) retrieveText(std::stoi(m_cookie[1]), ss_text); 
            else if(page_type == 1 && line.find("<?cky") != -1) ss_text << "<br><br>\n";
            
            //For some reason I do not understand, if you insert a <script> tag to declare the below JS variable outside of the script tag at the bottom, on Chrome Android the server very often will get stuck for absolutely ages on loading the Bookerly font file when you refresh the page, and the javascript engine will freeze for about 5-10seconds, but this never happens on Desktop. Thus I've had to make up another bullshit <?js tag to signal where to insert this C++-generated JS, and the only reason I'm inserting it server-side is because JS string functions are absolute dogshit and compared to my C-string parsing of the cookie text, doing it on the client by parsing the document.cookie string is ungodlily inefficient
            else if(page_type == 1 && cookies_present && line.find("<?js") != -1) ss_text << "let cookie_textselect = " + m_cookie[1] + ";\n";
            else if(page_type == 1 && line.find("<?js") != -1) ss_text << "let cookie_textselect = 0;\n";

            else ss_text << line << '\n';
        }

        content = ss_text.str();

        urlFile.close();
    }
    else
    {
        std::cout << "This file was not opened successfully." << std::endl;
    }
        
}


void WebServer::insertTextSelect(std::ostringstream &html) {
    sqlite3* DB;
    sqlite3_stmt* statement;

    if(!sqlite3_open(m_DB_path, &DB)) {
        int prep_code, run_code;
        const char *sql_text;

        sql_text = "SELECT text_id, text_title FROM texts";
        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);

        int text_id = 0;
        const char* text_title;
        std::string text_title_str;

        while(sqlite3_step(statement) == SQLITE_ROW) {
            text_id = sqlite3_column_int(statement, 0);
            text_title = (const char*)sqlite3_column_text(statement, 1);

            icu::UnicodeString unicode_text_title = text_title;
            unicode_text_title.findAndReplace("??", "\'");
            unicode_text_title.toUTF8String(text_title_str);

            html << "<option value=\"" << text_id << "\">" << text_title_str << "</option>\n";
            text_title_str = "";
        }

        sqlite3_finalize(statement);
    
        sqlite3_close(DB); 
    }
}

void WebServer::buildPOSTedData(const char* msg, bool headers_present, int length) {

    if(headers_present) {

        setURL(msg);
        
        int content_length_start = c_strFind(msg, "Content-Length:") + 16;
        
        int cl_figures = 0;
        char next_nl;
        while(next_nl != '\x0d') {
            next_nl = msg[content_length_start + cl_figures];
            cl_figures++;
        }
        cl_figures--;
        printf("Number of digits in content-length: %i\n", cl_figures);

        char content_length_str[cl_figures + 1];
        for(int i = 0; i < cl_figures; i++) {
            content_length_str[i] = msg[content_length_start + i];
        }
        content_length_str[cl_figures] = '\0';
        int content_length = atoi(content_length_str);
        printf("Parsed-out content-length: %i\n", content_length);

        int headers_length = c_strFind(msg, "\r\n\r\n");
        m_POST_continue = false;
        m_bytes_handled = 0;
        m_total_post_bytes = headers_length + 4 + content_length;
        if(m_total_post_bytes > length) {
            std::string post_data {msg + headers_length + 4}; //strcat won't work unless I use malloc() to allocate heap-memory of appropriate size for the stuck-together c-strings, so may as well use std::string if I'm forced to heap-allocate anyway
            //at least I'm only heap-allocating in those instances where the POST data makes the message over 4096 bytes
           m_post_data_incomplete = post_data;
           // std::cout << "headers_length + 4 + content_length > length" << std::endl;
           m_POST_continue = true;
           m_bytes_handled += length;
            
        }
        else {
            m_post_data = msg + headers_length + 4;
        }
  
    }
    else {
        
        std::string post_data_cont {msg};
        m_post_data_incomplete += post_data_cont;
        m_post_data = m_post_data_incomplete.c_str();
        m_bytes_handled += length;
        if(m_total_post_bytes == m_bytes_handled) {
            m_POST_continue = false;
        }    
    }

}

std::string WebServer::URIDecode(std::string &text) //stolen off a rando on stackexchange who forgot to null-terminate the char-array before he strtol()'d it, which caused big problems
{
    std::string escaped;

    for (auto i = text.begin(), nd = text.end(); i < nd; ++i)
    {
        auto c = (*i);

        switch (c)
        {
        case '%':
            if (i[1] && i[2])
            {
                char hs[]{i[1], i[2], '\0'};
                escaped += static_cast<char>(strtol(hs, nullptr, 16));
                i += 2;
            }
            break;
        case '+':
            escaped += ' ';
            break;
        default:
            escaped += c;
        }
    }

    return escaped;
}

std::string WebServer::htmlspecialchars(const std::string &innerHTML) {
    std::string escaped;

    for(auto i = innerHTML.begin(), nd = innerHTML.end(); i < nd; i++) {
        char c = (*i);

        switch(c) {
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '\'':
                escaped += "&#039;";
                break;
            case '&':
                escaped += "&amp;";
                break;
            default:
                escaped += c;
        }
    }
    return escaped;
}

std::string WebServer::escapeQuotes(const std::string &quoteystring) {
    std::string escaped;
    for(auto i = quoteystring.begin(), nd = quoteystring.end(); i < nd; i++) {

        char c = (*i); //the quotation mark is always a single-byte ASCII char so this is fine
        switch(c) {
            case '\x22':
                escaped += "\\\"";
                break;
            default:
                escaped += c;
        }
    }
    return escaped;
}

//in order to avoid using std::vector I cannot save the array of the POSTed data into a variable which persists outside this function, because the size of the array is only known if I know which POST data I am processing, which means the code to create an array out of the m_post_data has to be repeated everytime a function to handle it is called
void WebServer::handlePOSTedData(const char* post_data, int clientSocket) {
    printf("------------------------COMPLETE POST DATA------------------------\n%s\n-------------------------------------------------------\n", post_data);
    
    int post_fields = getPostFields(m_url);

    int equals_positions_arr[post_fields];
    int equals_pos = 0;
    for(int i = 0; i < post_fields; i++) {
        equals_pos = c_strFind(post_data + equals_pos + 1, "=") + equals_pos + 1;
        if(equals_pos == -1) break;
        equals_positions_arr[i] = equals_pos;
        
    }

    int amp_positions_arr[post_fields];
    int amp_pos = 0;
   
  
    for(int i = 0; i < post_fields - 1; i++) {
        amp_pos = c_strFind(post_data + amp_pos + 1, "&") + amp_pos + 1;
        if (amp_pos == -1) break;
        amp_positions_arr[i] = amp_pos;
        
    }
    amp_positions_arr[post_fields -1] = strlen(post_data);
    

    for(int i = 0; i < post_fields; i++) {
        printf("equals position no. %i: %i\n", i, equals_positions_arr[i]);
    }
    for(int i = 0; i < post_fields - 1; i++) {
        printf("amp position no. %i: %i\n", i, amp_positions_arr[i]);
    }
    std::string post_values[post_fields];
    for(int i = 0; i < post_fields; i++) {
        //this could be a really long text so if over a certain size needs to be heap-allocated (however it's also used for literally all POST requests, most of which are tiny, so the limit for stack-allocation will be 512Kb)
        int ith_post_value_length = amp_positions_arr[i] - equals_positions_arr[i] + 1;
        
        if(ith_post_value_length > 524288) {
            char* ith_post_value = new char[ith_post_value_length];
            printf("length of post_value array: %i\n", ith_post_value_length);
            for(int j = 0; j < amp_positions_arr[i] - equals_positions_arr[i] - 1; j++) {
                ith_post_value[j] = post_data[equals_positions_arr[i] + 1 + j];
            // printf("j = %i, char = %c | ", j, post_data[equals_positions_arr[i] + 1 + j]);
            }
            ith_post_value[amp_positions_arr[i] - equals_positions_arr[i] - 1] = '\0';
            
            std::string ith_post_value_str {ith_post_value};
            delete[] ith_post_value;
      
            post_values[i] = ith_post_value_str;
        }
        else {
            char ith_post_value[ith_post_value_length];  //In Windows I will make this probably a 4096 bytes fixed-sized-array, because no-one needs 512Kb to hold a lang_id etc.
            printf("length of post_value array: %i\n", ith_post_value_length);
            for(int j = 0; j < amp_positions_arr[i] - equals_positions_arr[i] - 1; j++) {
                ith_post_value[j] = post_data[equals_positions_arr[i] + 1 + j];
            // printf("j = %i, char = %c | ", j, post_data[equals_positions_arr[i] + 1 + j]);
            }
            ith_post_value[amp_positions_arr[i] - equals_positions_arr[i] - 1] = '\0';
            
            std::string ith_post_value_str {ith_post_value};
      
            post_values[i] = ith_post_value_str;
        }    
    }

    for (int i = 0; i < post_fields; i++)
    {
       std::cout << "POST value no." << i << ": " << post_values[i] << std::endl;
    }

    if(!strcmp(m_url, "/get_lang_id.php")) {
        bool php_func_success = getLangId(post_values, clientSocket);
    }
    if(!strcmp(m_url, "/retrieve_engword.php")) {
        bool php_func_success = retrieveEngword(post_values, clientSocket);
    }
    if(!strcmp(m_url, "/retrieve_meanings.php")) {
        bool php_func_success = retrieveMeanings(post_values, clientSocket);
    }
    if(!strcmp(m_url, "/lemma_tooltip.php")) {
        bool php_func_success = lemmaTooltips(post_values, clientSocket);
    }
    if(!strcmp(m_url, "/lemma_delete.php")) {
        bool php_func_success = deleteLemma(post_values, clientSocket);
    }
    if(!strcmp(m_url, "/lemma_record.php")) {
        bool php_func_success = recordLemma(post_values, clientSocket);
    }
    if(!strcmp(m_url, "/retrieve_text.php")) {
        bool php_func_success = retrieveText(post_values, clientSocket);
    }
    if(!strcmp(m_url, "/retrieve_text_splitup.php")) {
        bool php_func_success = retrieveTextSplitup(post_values, clientSocket);
    }
    if(!strcmp(m_url, "/delete_text.php")) {
        bool php_func_success = deleteText(post_values, clientSocket);
    }
    if(!strcmp(m_url, "/update_db.php")) {
        bool php_func_success = addText(post_values, clientSocket);  
    }
    if(!strcmp(m_url, "/pull_lemma.php")) {
        bool php_func_success = pullInLemma(post_values, clientSocket);
    }

    printf("m_url: %s\n", m_url);
    
}

void WebServer::setURL(const char* msg) {
    int url_start = c_strFind(msg, "/");
  //  printf("url_start: %i\n", url_start);
    int url_end = c_strFind(msg + url_start, " ") + url_start;
  //  printf("url_end: %i\n", url_end);
    char url[url_end - url_start + 1];
    for (int i = 0; i < url_end - url_start; i++)
    {
        url[i] = msg[url_start + i];
    }
    url[url_end - url_start] = '\0';
    
    strcpy(m_url, url); //m_url is max 50 chars but this is allowed because I tightly control what the POST urls are; using std::string would be wasteful
}

int WebServer::getPostFields(const char* url) {
    if(!strcmp(url, "/update_db.php")) return 3;
    else if(!strcmp(url, "/lemma_tooltip.php")) return 2;
    else if(!strcmp(url, "/retrieve_text.php")) return 1;
    else if(!strcmp(url, "/retrieve_text_splitup.php")) return 3;
    else if(!strcmp(url, "/get_lang_id.php")) return 1;
    else if(!strcmp(url, "/pull_lemma.php")) return 4;
    else if(!strcmp(url, "/lemma_record.php")) return 8;
    else if(!strcmp(url, "/lemma_delete.php")) return 3;
    else if(!strcmp(url, "/retrieve_engword.php")) return 3;
    else if(!strcmp(url, "/retrieve_meanings.php")) return 2;
    else if(!strcmp(url, "/delete_text.php")) return 1;
    else return 10;
}

bool WebServer::setCookie(std::string cookie[2], const char* msg) {
    int cookie_start = c_strFind(msg, "\r\nCookie") + 9;
    if(cookie_start == 8) return false; //c_strFind() returns -1 if unsuccessful, but I've just added 9 to it so the number signalling no cookies is 8

    int cookieName_start = c_strFind(msg + cookie_start, " text_id=");
    if(cookieName_start == -1) return false;

    cookie[0] = "text_id";

    int val_length = c_strFindNearest(msg + cookie_start + cookieName_start + 9, ";", "\r\n");
    std::cout << val_length << std::endl;

    char val[val_length + 1];
    for(int i = 0; i < val_length; i++) {
        val[i] = (msg + cookie_start + cookieName_start + 9)[i];
    }
    val[val_length] = '\0';

    cookie[1] = val;
    return true;
}

bool WebServer::addText(std::string _POST[3], int clientSocket) {
    std::cout << "update_db.php" << std::endl;
   
    UErrorCode status = U_ZERO_ERROR;
    std::string text_body = URIDecode(_POST[0]);

    sqlite3 *DB;
    sqlite3_stmt *statement;

    

    if(!sqlite3_open(m_DB_path, &DB)) {

     //   int count = 0;

        icu::BreakIterator *bi = icu::BreakIterator::createWordInstance(icu::Locale::getUS(), status);
        icu::UnicodeString regexp = "\\p{L}"; // from the ICU regex documentation: \p{UNICODE PROPERTY NAME}	Match any character having the specified Unicode Property. (L = letter). This will catch exactly the same things as what the BreakIterator views as word-boundaries, meaning words with thigns like the OCS abbreviation tilde which would get classified as non-words if we used \P{UNICODE PROPERTY NAME} (match characters NOT having the specified property). If the BreakIterator fucks up then this will fuck up, but in that case we were fucked anyway. possibly \w (match Unicode 'words') is better but that also matches numerals which I'm ambivalent about given there can be infinity numbers
        icu::RegexMatcher *matcher = new icu::RegexMatcher(regexp, 0, status); //call delete on this
        
        
        std::istringstream iss(text_body);

        int prep_code, run_code;
        const char *sql_commands[1];
        std::string sql_text_str;
        const char *sql_text;

        int lang_id = std::stoi(_POST[2]);

        
   
        sql_text = "SELECT MAX(tokno) FROM display_text";
        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        run_code = sqlite3_step(statement);
             
        sqlite3_int64 dt_start = sqlite3_column_int64(statement, 0) + 1;
        sqlite3_finalize(statement);

        const char *sql_BEGIN = "BEGIN IMMEDIATE";
        const char *sql_COMMIT = "COMMIT";
              
        prep_code = sqlite3_prepare_v2(DB, sql_BEGIN, -1, &statement, NULL);
        run_code = sqlite3_step(statement);
        sqlite3_finalize(statement);

        

        for(std::string token; std::getline(iss, token, ' '); ) {
            icu::UnicodeString token_unicode = token.c_str();

            token_unicode.findAndReplace("'", "??");
            token_unicode.findAndReplace("???", "??");

            bi->setText(token_unicode);

            int32_t start_range, end_range;
            icu::UnicodeString tb_copy;
            std::string chunk;

            std::string text_word;

            int32_t p = bi->first();
            bool is_a_word = true;
           
            while(p != icu::BreakIterator::DONE) {
                tb_copy = token_unicode;
                start_range = p;
                p = bi->next();
                end_range = p;
                if(end_range == -1) break;
                tb_copy.retainBetween(start_range, end_range);
                
                matcher->reset(tb_copy); //tells the RegexMatcher object to use tb_copy as its haystack
                if(matcher->find()) {
                    is_a_word = true;
                }
                else {
                    is_a_word = false;
                }
                
                if(is_a_word) {          
                
                    tb_copy.toUTF8String(text_word);
                    if(_POST[2] == "7") {
                        tb_copy.toLower(icu::Locale("tr", "TR"));
                    }
                    else{
                        tb_copy.toLower();
                    }
                    
                    tb_copy.toUTF8String(chunk);

                    sql_text_str = "INSERT OR IGNORE INTO word_engine (word, lang_id) VALUES (\'"+chunk+"\', "+_POST[2]+")";
                    //std::cout << sql_text_str << std::endl;
                    sql_text = sql_text_str.c_str();
                    sql_commands[0] = sql_text;

                    for (int i = 0; i < 1; i++) {
                        prep_code = sqlite3_prepare_v2(DB, sql_commands[i], -1, &statement, NULL);
                        run_code = sqlite3_step(statement);
                        sqlite3_finalize(statement);
                    //   std::cout << prep_code << std::endl;
                    //    std::cout << run_code << std::endl;
                    }

                    sql_text_str = "INSERT INTO display_text (word_engine_id, text_word) SELECT word_engine_id, \'"+text_word+"\' FROM word_engine WHERE word = \'"+chunk+"\' AND lang_id = "+_POST[2];
                    // std::cout << sql_text_str << std::endl;
                    sql_text = sql_text_str.c_str();
                    prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
                    run_code = sqlite3_step(statement);
                    sqlite3_finalize(statement); 
                }

                else {
                    tb_copy.toUTF8String(text_word);

                    sql_text_str = "INSERT INTO display_text (text_word) VALUES (\'"+text_word+"\')";
                    // std::cout << sql_text_str << std::endl;
                    sql_text = sql_text_str.c_str();
                    prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
                    run_code = sqlite3_step(statement);
                    sqlite3_finalize(statement);
                }
                chunk = ""; //toUTF8String appends the string in tb_copy to chunk rather than overwriting it
                text_word = "";
            }
            
            sql_text_str = "UPDATE display_text SET space = 1 WHERE tokno = last_insert_rowid()";
          //  std::cout << sql_text_str << std::endl;
            sql_text = sql_text_str.c_str();
            prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
            run_code = sqlite3_step(statement);
            sqlite3_finalize(statement);
        }

        delete bi;
        delete matcher;
        
        icu::UnicodeString unicode_text_title = URIDecode(_POST[1]).c_str();
        std::string text_title;
        unicode_text_title.findAndReplace("'", "??");
        unicode_text_title.findAndReplace("???", "??");
        unicode_text_title.toUTF8String(text_title);
        const char* text_title_c_str = text_title.c_str();
       

        sql_text = "INSERT INTO texts (text_title, dt_start, dt_end, lang_id) VALUES (?, ?, last_insert_rowid(), ?)";
        std::cout << "dt_start: " << dt_start << ", text_title: " << text_title << ", last_insert_row_id: " << sqlite3_last_insert_rowid(DB) << std::endl;

        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        sqlite3_bind_int64(statement, 2, dt_start);
        sqlite3_bind_text(statement, 1, text_title_c_str, -1, SQLITE_STATIC);
        sqlite3_bind_int(statement, 3, lang_id);

        run_code = sqlite3_step(statement);
        std::cout << "Finalize code: " << sqlite3_finalize(statement) << std::endl;
        std::cout << prep_code << " " << run_code << std::endl;

      
        prep_code = sqlite3_prepare_v2(DB, sql_COMMIT, -1, &statement, NULL);
        run_code = sqlite3_step(statement);
        std::cout << prep_code << " " << run_code << std::endl;

        sqlite3_finalize(statement);

       

        std::cout << "sqlite_close: " << sqlite3_close(DB) << std::endl;
    }  
    std::string POST_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
    int size = POST_response.size() + 1;

    sendToClient(clientSocket, POST_response.c_str(), size);
    return true;
}

bool WebServer::deleteText(std::string _POST[1], int clientSocket) {
    int text_id = std::stoi(_POST[0]);
    
    UErrorCode status = U_ZERO_ERROR;
    sqlite3* DB;
    sqlite3_stmt* statement;

    if(!sqlite3_open(m_DB_path, &DB)) {
    int prep_code, run_code;
    const char* sql_text;
    
    sql_text = "SELECT MAX(text_id) FROM texts";
    prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
    run_code = sqlite3_step(statement);
    int end_text_id = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);

    bool end_text = (end_text_id == text_id);

    sql_text = "SELECT dt_start, dt_end FROM texts WHERE text_id = ?";
    prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
    sqlite3_bind_int(statement, 1, text_id);
    run_code = sqlite3_step(statement);
    std::cout << prep_code << " " << run_code << std::endl;

    sqlite3_int64 dt_start = sqlite3_column_int64(statement, 0);
    sqlite3_int64 dt_end = sqlite3_column_int64(statement, 1);
    sqlite3_finalize(statement);

    sql_text = "DELETE FROM display_text WHERE tokno >= ? AND tokno <= ?";
    prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
    sqlite3_bind_int64(statement, 1, dt_start);
    sqlite3_bind_int64(statement, 2, dt_end);
    run_code = sqlite3_step(statement);
    std::cout << prep_code << " " << run_code << std::endl;
    sqlite3_finalize(statement);

    sql_text = "DELETE FROM texts WHERE text_id = ?";
    prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
    sqlite3_bind_int(statement, 1, text_id);
    run_code = sqlite3_step(statement);
    std::cout << prep_code << " " << run_code << std::endl;
    sqlite3_finalize(statement);

    sqlite3_close(DB);
   
    std::cout << dt_start << "-->" << dt_end << std::endl;
    
    }

    std::string POST_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 0\r\nSet-Cookie: text_id=" + _POST[0] + "; Max-Age=0\r\n\r\n";
    int size = POST_response.size() + 1;

    sendToClient(clientSocket, POST_response.c_str(), size);
    return true;


}

bool WebServer::lemmaTooltips(std::string _POST[2], int clientSocket) {
    sqlite3* DB;    

    if(!sqlite3_open(m_DB_path, &DB)) {

        sqlite3_stmt* statement1;
        sqlite3_stmt* statement2;
        sqlite3_stmt* statement3; 
        sqlite3_stmt* statement4;      

        int tooltip_count = 1;
        //word_engine_id's are generally going to be smaller than toknos so more efficient to iterate through them
        for(auto i = _POST[1].begin(), nd=_POST[1].end(); i < nd; i++) {
            char c = (*i);
            if(c == ',') tooltip_count++;
        }

        std::vector<sqlite3_int64> toknos;
        std::vector<int> word_engine_ids;

        toknos.reserve(tooltip_count); 
        word_engine_ids.reserve(tooltip_count);

        std::string tokno_str = "";
        for(auto i = _POST[0].begin(), nd=_POST[0].end(); i < nd; i++) {
            char c = (*i);
            if(c == ',') {
                sqlite3_int64 tokno = std::stoi(tokno_str);
                toknos.emplace_back(tokno);
                tokno_str = "";
                continue;
            }
            tokno_str += c;
            if(nd - 1 == i) {
                sqlite3_int64 tokno = std::stoi(tokno_str);
                toknos.emplace_back(tokno);
            }
        }
        std::string word_engine_id_str = "";
        for(auto i = _POST[1].begin(), nd=_POST[1].end(); i < nd; i++) {
            char c = (*i);
            if(c == ',') {
                int word_engine_id = std::stoi(word_engine_id_str);
                word_engine_ids.emplace_back(word_engine_id);
                word_engine_id_str = "";
                continue;
            }
            word_engine_id_str += c;
            if(nd - 1 == i) {
                int word_engine_id = std::stoi(word_engine_id_str);
                word_engine_ids.emplace_back(word_engine_id);
            }
        }

        const char* sql_text1 = "SELECT lemma_id, lemma_meaning_no FROM display_text WHERE tokno = ?";
        const char* sql_text2 = "SELECT first_lemma_id FROM word_engine WHERE word_engine_id = ?";
        const char* sql_text3 = "SELECT lemma, eng_trans1, eng_trans2, eng_trans3, eng_trans4, eng_trans5, eng_trans6, eng_trans7, eng_trans8, eng_trans9, eng_trans10, pos FROM lemmas WHERE lemma_id = ?";
        sqlite3_prepare_v2(DB, sql_text1, -1, &statement1, NULL);
        sqlite3_prepare_v2(DB, sql_text2, -1, &statement2, NULL);
        sqlite3_prepare_v2(DB, sql_text3, -1, &statement3, NULL);

        std::string lemma_form = "";
        std::string lemma_trans = "";
        short int pos = 1;


        std::ostringstream json;
        json << "[";

        int x = 0;
        for(const sqlite3_int64 &tokno : toknos) {

            sqlite3_bind_int64(statement1, 1, tokno);
            sqlite3_step(statement1);
            int lemma_id = sqlite3_column_int(statement1, 0);
 
            if(lemma_id) {
                short int lemma_meaning_no = sqlite3_column_int(statement1, 1);
                std::string sql_text4_str = "SELECT lemma, eng_trans"+std::to_string(lemma_meaning_no)+", pos FROM lemmas WHERE lemma_id = "+std::to_string(lemma_id); 
                sqlite3_prepare_v2(DB, sql_text4_str.c_str(), -1, &statement4, NULL);
                sqlite3_step(statement4);

                const unsigned char* lemma_rawsql = sqlite3_column_text(statement4, 0);
                if(lemma_rawsql != nullptr) {
                    lemma_form = (const char*)lemma_rawsql;
                }
                else lemma_form = "";
                const unsigned char* lemma_trans_rawsql = sqlite3_column_text(statement4, 1);
                if(lemma_trans_rawsql != nullptr) {
                    lemma_trans = (const char*)lemma_trans_rawsql;
                }
                else lemma_trans = "";
                pos = sqlite3_column_int(statement4, 2);

                std::cout << "lemma_form: " << lemma_form << ", lemma_trans: " << lemma_trans << ", pos: " << pos << std::endl;
                //sqlite3_reset(statement1);
                sqlite3_finalize(statement4);
            }
            else {
                sqlite3_bind_int(statement2, 1, word_engine_ids[x]);
                sqlite3_step(statement2);
                lemma_id = sqlite3_column_int(statement2, 0);
                std::cout << "lemma_id: " << lemma_id << std::endl;
                sqlite3_reset(statement2);

                sqlite3_bind_int(statement3, 1, lemma_id);
                sqlite3_step(statement3);
                const unsigned char* lemma_rawsql = sqlite3_column_text(statement3, 0);
                if(lemma_rawsql != nullptr) {
                    lemma_form = (const char*)lemma_rawsql;
                }
                else lemma_form = "";

                lemma_trans = "";
                for(int i = 1; i < 11; i++) {
                    const unsigned char* lemma_trans_rawsql = sqlite3_column_text(statement3, i);
                    if(lemma_trans_rawsql != nullptr) {
                        lemma_trans = (const char*)lemma_trans_rawsql;
                        break;
                    }
                }
                pos = sqlite3_column_int(statement3, 11);
                sqlite3_reset(statement3);
                std::cout << "lemma_form: " << lemma_form << ", lemma_trans: " << lemma_trans << ", pos: " << pos << std::endl;     
            }
            //I'm sure these need to be htmlspecialchars()'d as well/instead (as that escapes quotes with html codes)
            json << "{\"lemma_form\":\"" << htmlspecialchars(lemma_form) << "\",\"lemma_trans\":\"" << htmlspecialchars(lemma_trans) << "\",\"pos\":\"" << pos << "\"}";
            x++;
            if(x != tooltip_count) {
                json << ",";
            } //for some reason the browser only recognises the response as JSON if there is no trailing comma
            sqlite3_reset(statement1);
        } 
        sqlite3_finalize(statement1);
        sqlite3_finalize(statement2);
        sqlite3_finalize(statement3);

        sqlite3_close(DB);

        json << "]";
        int content_length = json.str().size();

        std::ostringstream post_response;
        post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << json.str();

        int length = post_response.str().size() + 1;
        sendToClient(clientSocket, post_response.str().c_str(), length);
       
        return true;
    }
    else {
        std::cout << "Database connection failed in lemmaTooltips()" << std::endl;
        return false;
    }
}

bool WebServer::retrieveText(std::string text_id[1], int clientSocket) {

    int text_id_int = std::stoi(text_id[0]);
    std::cout << text_id_int << std::endl;

    if(text_id_int == 0) {
       std::ostringstream html;
       html << "<br><br>";

       std::string content_str = html.str();
       int content_length = content_str.size();

       std::ostringstream post_response_ss;
       post_response_ss << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length;
       post_response_ss << "\r\n" << "Set-Cookie: text_id=" << text_id[0];
       post_response_ss << "\r\n\r\n" << content_str;
       int length = post_response_ss.str().size() + 1;
       sendToClient(clientSocket, post_response_ss.str().c_str(), length);
       std::cout << "Sent text to client" << std::endl;
       return true;
    }

    UErrorCode status = U_ZERO_ERROR;
    sqlite3* DB;
    sqlite3_stmt* statement;

    std::ostringstream html;
   

    if(!sqlite3_open(m_DB_path, &DB)) {
    
    int prep_code, run_code;
    const char *sql_text;

    sql_text = "SELECT dt_start, dt_end, text_title, lang_id FROM texts WHERE text_id = ?";
    prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
    std::cout << "bind code: " << sqlite3_bind_int(statement, 1, text_id_int) << std::endl;
    run_code = sqlite3_step(statement);

    std::cout << prep_code << " " << run_code << std::endl;

    sqlite3_int64 dt_start = sqlite3_column_int64(statement, 0);
    sqlite3_int64 dt_end = sqlite3_column_int64(statement, 1);
    int lang_id = sqlite3_column_int(statement, 3);
    
    
        const char* text_title = (const char*)sqlite3_column_text(statement, 2);
        icu::UnicodeString text_title_utf8 = text_title;
        text_title_utf8.findAndReplace("??", "\'");
        std::string text_title_str;
        text_title_utf8.toUTF8String(text_title_str);
        html << "<h1 id=\"title\">" << text_title_str << "</h1><br><div id=\"textbody\">&emsp;";
    
  /*  else {
        html << "<h1 id=\"title\">" << sqlite3_column_text(statement, 2) << "</h1><br><div id=\"textbody\">&emsp;";
    } */
    sqlite3_finalize(statement);

    sql_text = "SELECT count(*) FROM display_text WHERE tokno >= ? AND tokno <= ? AND (space = 1 OR text_word = '\n')";
    prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
    sqlite3_bind_int64(statement, 1, dt_start);
    sqlite3_bind_int64(statement, 2, dt_end);
    run_code = sqlite3_step(statement);
    int chunk_total = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    std::cout << "Total number of chunks in this text: " << chunk_total << std::endl;
    
    sql_text = "SELECT * FROM display_text WHERE tokno >= ? AND tokno <= ?";
    prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
    sqlite3_bind_int64(statement, 1, dt_start);
    sqlite3_bind_int64(statement, 2, dt_end);
    //run_code = sqlite3_step(statement);

    int chunk_count {1};
    float words_per_page {750};
    sqlite3_int64 tokno;
    int space, word_engine_id, lemma_meaning_no, lemma_id;

    sqlite3_stmt* stmt;
    const char* sql_word_eng = "SELECT first_lemma_id FROM word_engine WHERE word_engine_id = ?";
    sqlite3_prepare_v2(DB, sql_word_eng, -1, &stmt, NULL);

    int first_lemma_id;
    const char* text_word;
    bool newline = false;
    bool following_a_space = true;

    while(sqlite3_step(statement) == SQLITE_ROW && chunk_count <= words_per_page) {
        tokno = sqlite3_column_int64(statement, 0);
        space = sqlite3_column_int(statement, 2);
        word_engine_id = sqlite3_column_int(statement, 3);
        text_word = (const char*)sqlite3_column_text(statement, 1);

        newline = false;
        if(following_a_space) {
            html << "<span class=\"chunk\">";
        }

        if(word_engine_id) {
            lemma_id = sqlite3_column_int(statement, 5);
            lemma_meaning_no = sqlite3_column_int(statement, 4);

            
            sqlite3_bind_int(stmt, 1, word_engine_id);
            sqlite3_step(stmt);
            first_lemma_id = sqlite3_column_int(stmt, 0);
            sqlite3_reset(stmt);

            if(lemma_id) {
                html << "<span class=\"tooltip lemma_set_unexplicit lemma_set\" data-word_engine_id=\"" << word_engine_id << "\" data-tokno=\"" << tokno << "\">";
            }
            else if(first_lemma_id) {
                html << "<span class=\"tooltip lemma_set_unexplicit\" data-word_engine_id=\"" << word_engine_id << "\" data-tokno=\"" << tokno << "\">";
            }
            else {
                html << "<span class=\"tooltip\" data-word_engine_id=\"" << word_engine_id << "\" data-tokno=\"" << tokno << "\">";
            }
        }
        else {
            if(!strcmp(text_word, "??")) {
                text_word = "'";
            }
            else if(!strcmp(text_word, "\n")) {
                text_word = "<br>";
                chunk_count++;
                newline = true;
            }
        }
        html << text_word;
        if(word_engine_id) {
            html << "</span>";
        }
        if(space == 1 || newline) {
            html << "</span>";
            following_a_space = true;
        }
        else following_a_space = false;
        if(space == 1) {
            html << " ";
            chunk_count++;
        }
            

    }
    sqlite3_finalize(stmt);
    sqlite3_finalize(statement);
    html << "</div>";
    

    if(chunk_total > words_per_page) {
        html << "<br><br><div id=\"pagenos\">";

        chunk_count = 1;
        int pagenos = (int)ceil(chunk_total/words_per_page);
        sqlite3_int64 page_toknos[pagenos];
        
        
        page_toknos[0] = dt_start;

        int arr_index = 1;

        sql_text = "SELECT tokno FROM display_text WHERE tokno >= ? AND tokno <= ? AND (space = 1 OR text_word = '\n')";
        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        sqlite3_bind_int64(statement, 1, dt_start);
        sqlite3_bind_int64(statement, 2, dt_end);

        while(sqlite3_step(statement) == SQLITE_ROW) {
          
            tokno = sqlite3_column_int64(statement, 0);
            
            if(chunk_count % 750 == 0) {
                page_toknos[arr_index] = tokno + 1;
                std::cout << "arr_index: " << arr_index << std::endl;
                std::cout << "chunk_count: " << chunk_count << std::endl;
                arr_index++;
                
            }
            chunk_count++;
        }
        
        sqlite3_finalize(statement);

        for(int i = 0; i < pagenos; i++) {
            std::cout << "Page " << i + 1 << " starting tokno: " << page_toknos[i] << std::endl;
            html << "<span class=\"pageno\" onclick=\"selectText_splitup("<< page_toknos[i] << ", " << dt_end << ", " << i + 1 << ")\">" << i + 1 << "</span>";
        }
        html << "</div>";
    }
    else {
        html << "<br>";
    }

    sqlite3_close(DB); 

    std::string content_str = html.str();
    int content_length = content_str.size();

    std::ostringstream post_response_ss;
    post_response_ss << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length;
    post_response_ss << "\r\n" << "Set-Cookie: text_id=" << text_id[0];
    post_response_ss << "\r\n\r\n" << content_str;
    int length = post_response_ss.str().size() + 1;
    sendToClient(clientSocket, post_response_ss.str().c_str(), length);
    std::cout << "Sent text to client" << std::endl;
    return true;

    }
    else {       
        
        return false; 
    }
}

bool WebServer::retrieveTextSplitup(std::string _POST[3], int clientSocket) {
    UErrorCode status = U_ZERO_ERROR;
    sqlite3 *DB;
    sqlite3_stmt *statement;

    std::ostringstream html;

    if (!sqlite3_open(m_DB_path, &DB)) {

        int prep_code, run_code;
        const char *sql_text;

        sqlite3_int64 dt_start = std::stoi(_POST[0]);
        sqlite3_int64 dt_end = std::stoi(_POST[1]);
        int page_cur = std::stoi(_POST[2]);

        if(page_cur == 1) {
            html << "&emsp;";
        }

        sql_text = "SELECT * FROM display_text WHERE tokno >= ? AND tokno <= ?";
        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        sqlite3_bind_int64(statement, 1, dt_start);
        sqlite3_bind_int64(statement, 2, dt_end);
        // run_code = sqlite3_step(statement);

        int chunk_count{1};
        float words_per_page{750};
        sqlite3_int64 tokno;
        int space, word_engine_id, lemma_meaning_no, lemma_id;

        sqlite3_stmt *stmt;
        const char *sql_word_eng = "SELECT first_lemma_id FROM word_engine WHERE word_engine_id = ?";
        sqlite3_prepare_v2(DB, sql_word_eng, -1, &stmt, NULL);
        int first_lemma_id;
        const char *text_word;

        bool newline = false;
        bool following_a_space = true;

        while(sqlite3_step(statement) == SQLITE_ROW && chunk_count <= words_per_page) {
            tokno = sqlite3_column_int64(statement, 0);
            space = sqlite3_column_int(statement, 2);
            word_engine_id = sqlite3_column_int(statement, 3);
            text_word = (const char*)sqlite3_column_text(statement, 1);

            newline = false;
            if(following_a_space) {
                html << "<span class=\"chunk\">";
            }
            

            if(word_engine_id) {
                lemma_id = sqlite3_column_int(statement, 5);
                lemma_meaning_no = sqlite3_column_int(statement, 4);

                
                sqlite3_bind_int(stmt, 1, word_engine_id);
                sqlite3_step(stmt);
                first_lemma_id = sqlite3_column_int(stmt, 0);
                sqlite3_reset(stmt);

                if(lemma_id) {
                    html << "<span class=\"tooltip lemma_set_unexplicit lemma_set\" data-word_engine_id=\"" << word_engine_id << "\" data-tokno=\"" << tokno << "\">";
                }
                else if(first_lemma_id) {
                    html << "<span class=\"tooltip lemma_set_unexplicit\" data-word_engine_id=\"" << word_engine_id << "\" data-tokno=\"" << tokno << "\">";
                }
                else {
                    html << "<span class=\"tooltip\" data-word_engine_id=\"" << word_engine_id << "\" data-tokno=\"" << tokno << "\">";
                }
            }
            else {
                if(!strcmp(text_word, "??")) {
                    text_word = "'";
                }
                else if(!strcmp(text_word, "\n")) {
                    text_word = "<br>";
                    chunk_count++;
                    newline = true;
                }
            }
            html << text_word;
            if(word_engine_id) {
                html << "</span>";
            }
            if(space == 1 || newline) {
            html << "</span>";
            following_a_space = true;
            }
            else following_a_space = false;
            if(space == 1) {
                html << " ";
                chunk_count++;
            }
            

        }
        sqlite3_finalize(statement);
        sqlite3_finalize(stmt);
        sqlite3_close(DB);
        std::string content_str = html.str();
        int content_length = content_str.size();

        std::ostringstream post_response_ss;
        post_response_ss << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << content_str;
        int length = post_response_ss.str().size() + 1;
        sendToClient(clientSocket, post_response_ss.str().c_str(), length);
        std::cout << "Sent text to client" << std::endl;
        return true;
    }

    else return false;
    }

bool WebServer::getLangId(std::string text_id[1], int clientSocket) {

    sqlite3* DB;
    sqlite3_stmt* statement;
  

    if(!sqlite3_open(m_DB_path, &DB)) {
    
    int prep_code, run_code;
    
    std::string sql_text_str = "SELECT lang_id FROM texts WHERE text_id = "+text_id[0];
    const char* sql_text = sql_text_str.c_str();
    prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
    run_code = sqlite3_step(statement);
   
    int lang_id = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);

    int content_length = lang_id > 9 ? 2 : 1;


    std::ostringstream html;
    html << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << lang_id;
    int length = html.str().size() + 1;

    sqlite3_close(DB);

    sendToClient(clientSocket, html.str().c_str(), length);
    
    return true;
    }
    else return false;
}

void WebServer::retrieveText(int cookie_textselect, std::ostringstream &html) {

    std::cout << cookie_textselect << std::endl;

    if(cookie_textselect == 0) {
        html << "<br><br>\n";
        return;
    }

    UErrorCode status = U_ZERO_ERROR;
    sqlite3* DB;
    sqlite3_stmt* statement;
   

    if(!sqlite3_open(m_DB_path, &DB)) {
    
        int prep_code, run_code;
        const char *sql_text;

        sql_text = "SELECT dt_start, dt_end, text_title, lang_id FROM texts WHERE text_id = ?";
        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        std::cout << "bind code: " << sqlite3_bind_int(statement, 1, cookie_textselect) << std::endl;
        run_code = sqlite3_step(statement);

        std::cout << prep_code << " " << run_code << std::endl;

        sqlite3_int64 dt_start = sqlite3_column_int64(statement, 0);
        //if the cookie refers to a deleted text then SQLite will convert the null given by this query of an empty row to 0, which is falsey in C++
        if(!dt_start) {
            html << "<br><br>\n";
            sqlite3_finalize(statement);
            sqlite3_close(DB);
            return;
        }
        sqlite3_int64 dt_end = sqlite3_column_int64(statement, 1);
        int lang_id = sqlite3_column_int(statement, 3);
        
        
            const char* text_title = (const char*)sqlite3_column_text(statement, 2);
            icu::UnicodeString text_title_utf8 = text_title;
            text_title_utf8.findAndReplace("??", "\'");
            std::string text_title_str;
            text_title_utf8.toUTF8String(text_title_str);
            html << "<h1 id=\"title\">" << text_title_str << "</h1><br><div id=\"textbody\">&emsp;";
        
     /*   else {
            html << "<h1 id=\"title\">" << sqlite3_column_text(statement, 2) << "</h1><br><div id=\"textbody\">&emsp;";
        } */
        sqlite3_finalize(statement);

        sql_text = "SELECT count(*) FROM display_text WHERE tokno >= ? AND tokno <= ? AND (space = 1 OR text_word = '\n')";
        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        sqlite3_bind_int64(statement, 1, dt_start);
        sqlite3_bind_int64(statement, 2, dt_end);
        run_code = sqlite3_step(statement);
        int chunk_total = sqlite3_column_int(statement, 0);
        sqlite3_finalize(statement);
        std::cout << "Total number of chunks in this text: " << chunk_total << std::endl;
        
        sql_text = "SELECT * FROM display_text WHERE tokno >= ? AND tokno <= ?";
        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        sqlite3_bind_int64(statement, 1, dt_start);
        sqlite3_bind_int64(statement, 2, dt_end);
        //run_code = sqlite3_step(statement);

        int chunk_count {1};
        float words_per_page {750};
        sqlite3_int64 tokno;
        int space, word_engine_id, lemma_meaning_no, lemma_id;

        sqlite3_stmt* stmt;
        const char* sql_word_eng = "SELECT first_lemma_id FROM word_engine WHERE word_engine_id = ?";
        sqlite3_prepare_v2(DB, sql_word_eng, -1, &stmt, NULL);
        int first_lemma_id;
        const char* text_word;
        bool newline = false;
        bool following_a_space = true;

        while(sqlite3_step(statement) == SQLITE_ROW && chunk_count <= words_per_page) {
            tokno = sqlite3_column_int64(statement, 0);
            space = sqlite3_column_int(statement, 2);
            word_engine_id = sqlite3_column_int(statement, 3);
            text_word = (const char*)sqlite3_column_text(statement, 1);

            newline = false;
            if(following_a_space) {
                html << "<span class=\"chunk\">";
            }

            if(word_engine_id) {
                lemma_id = sqlite3_column_int(statement, 5);
                lemma_meaning_no = sqlite3_column_int(statement, 4);

                
                sqlite3_bind_int(stmt, 1, word_engine_id);
                sqlite3_step(stmt);
                first_lemma_id = sqlite3_column_int(stmt, 0);
                sqlite3_reset(stmt);

                if(lemma_id) {
                    html << "<span class=\"tooltip lemma_set_unexplicit lemma_set\" data-word_engine_id=\"" << word_engine_id << "\" data-tokno=\"" << tokno << "\">";
                }
                else if(first_lemma_id) {
                    html << "<span class=\"tooltip lemma_set_unexplicit\" data-word_engine_id=\"" << word_engine_id << "\" data-tokno=\"" << tokno << "\">";
                }
                else {
                    html << "<span class=\"tooltip\" data-word_engine_id=\"" << word_engine_id << "\" data-tokno=\"" << tokno << "\">";
                }
            }
            else {
                if(!strcmp(text_word, "??")) {
                    text_word = "'";
                }
                else if(!strcmp(text_word, "\n")) {
                    text_word = "<br>";
                    chunk_count++;
                    newline = true;
                }
            }
            html << text_word;
            if(word_engine_id) {
                html << "</span>";
            }
            if(space == 1 || newline) {
                html << "</span>";
                following_a_space = true;
            }
            else following_a_space = false;
            if(space == 1) {
                html << " ";
                chunk_count++;
            }
                

        }
        sqlite3_finalize(stmt);
        sqlite3_finalize(statement);
        html << "</div>";
        

        if(chunk_total > words_per_page) {
            html << "<br><br><div id=\"pagenos\">";

            chunk_count = 1;
            int pagenos = (int)ceil(chunk_total/words_per_page);
            sqlite3_int64 page_toknos[pagenos];
            
            
            page_toknos[0] = dt_start;

            int arr_index = 1;

            sql_text = "SELECT tokno FROM display_text WHERE tokno >= ? AND tokno <= ? AND (space = 1 OR text_word = '\n')";
            prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
            sqlite3_bind_int64(statement, 1, dt_start);
            sqlite3_bind_int64(statement, 2, dt_end);

            while(sqlite3_step(statement) == SQLITE_ROW) {
            
                tokno = sqlite3_column_int64(statement, 0);
                
                if(chunk_count % 750 == 0) {
                    page_toknos[arr_index] = tokno + 1;
                    std::cout << "arr_index: " << arr_index << std::endl;
                    std::cout << "chunk_count: " << chunk_count << std::endl;
                    arr_index++;
                    
                }
                chunk_count++;
            }
            
            sqlite3_finalize(statement);

            for(int i = 0; i < pagenos; i++) {
                std::cout << "Page " << i + 1 << " starting tokno: " << page_toknos[i] << std::endl;
                html << "<span class=\"pageno\" onclick=\"selectText_splitup("<< page_toknos[i] << ", " << dt_end << ", " << i + 1 << ")\">" << i + 1 << "</span>";
            }
            html << "</div>";
        }
        else {
            html << "<br>";
        }

        sqlite3_close(DB); 

    }
    else {       
        
        html << "Database connection failed<br><br>\n";
    }
}


void WebServer::sendFontFile(char* url_c_str, int clientSocket) {

    std::ifstream urlFile(url_c_str, std::ios::binary);
    if (urlFile.good())
    {
        std::cout << "This file exists and was opened successfully." << std::endl;

        struct stat size_result;    
        int font_filesize = 0;
        if(stat(url_c_str, &size_result) == 0) {
            font_filesize = size_result.st_size;
            std::cout << "Fontfile size: " << font_filesize << " bytes" << std::endl;
        }
        else {
            std::cout << "Error reading fontfile size" << std::endl;
            return;
        }
        std::string headers =  "HTTP/1.1 200 OK\r\nContent-Type: font/ttf\r\nContent-Length: "+ std::to_string(font_filesize) + "\r\n\r\n";
        int headers_size = headers.size();
        const char* headers_c_str = headers.c_str();

        if(font_filesize < 1048576) {
            char content_buf[headers_size + font_filesize + 1];

            memcpy(content_buf, headers_c_str, headers_size); //.size() leaves off null-termination in its count

            urlFile.read(content_buf + headers_size, font_filesize);

            content_buf[headers_size + font_filesize] = '\0';

            sendToClient(clientSocket, content_buf, headers_size + font_filesize);

            urlFile.close();
        }
        else {
            char* content_buf = new char[headers_size + font_filesize + 1];

            memcpy(content_buf, headers_c_str, headers_size); //.size() leaves off null-termination in its count

            urlFile.read(content_buf + headers_size, font_filesize);

            content_buf[headers_size + font_filesize] = '\0';

            sendToClient(clientSocket, content_buf, headers_size + font_filesize);

            delete[] content_buf;
            urlFile.close();
        }      
    }
    else
    {
        std::cout << "This file was not opened successfully." << std::endl;
        return;
    }
    
}

bool WebServer::retrieveEngword(std::string _POST[3], int clientSocket) {
    
    UErrorCode status = U_ZERO_ERROR;
    sqlite3 *DB;
    sqlite3_stmt *statement;

    if(!sqlite3_open(m_DB_path, &DB)) {

        int word_engine_id = std::stoi(_POST[0]);
        sqlite_int64 tokno_current = std::stoi(_POST[1]);
        int lang_id = std::stoi(_POST[2]);

        const char* word_eng_word;
        int first_lemma_id = 0;

        int existing_lemma_id = 0;

        int prep_code, run_code;

        const char* sql_text = "SELECT word, first_lemma_id FROM word_engine WHERE word_engine_id = ?";
        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        sqlite3_bind_int(statement, 1, word_engine_id);
        run_code = sqlite3_step(statement);
        
        word_eng_word = (const char*)sqlite3_column_text(statement, 0);
        std::string word_eng_word_str = "";
        if(word_eng_word != nullptr) word_eng_word_str = word_eng_word;

        first_lemma_id = sqlite3_column_int(statement, 1); //will be 0 if the field is empty
        sqlite3_finalize(statement);
        short int lemma_meaning_no = 0;

        sql_text = "SELECT lemma_id, lemma_meaning_no FROM display_text WHERE tokno = ?";
        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        sqlite3_bind_int64(statement, 1, tokno_current);
        run_code = sqlite3_step(statement);
        existing_lemma_id = sqlite3_column_int(statement, 0);
        lemma_meaning_no = sqlite3_column_int(statement, 1);
        sqlite3_finalize(statement);

        std::cout << "word_eng_word = " << word_eng_word_str << "; first_lemma_id = " << first_lemma_id << "; existing_lemma_id = " << existing_lemma_id << std::endl;

        std::string lemma_tag_content = "";
        std::string lemma_textarea_content = "";
        int lemma_id = 0;
     
        short int pos = 1;

        if(!first_lemma_id) {
            lemma_tag_content = word_eng_word_str;

            //could return more than one row but we just take the first as the default
            std::string sql_text_str = "SELECT eng_trans1, pos FROM lemmas WHERE lemma = \'"+word_eng_word_str+"\' AND lang_id = "+_POST[2];
            prep_code = sqlite3_prepare_v2(DB, sql_text_str.c_str(), -1, &statement, NULL);
            run_code = sqlite3_step(statement);
            const unsigned char* lemma_textarea_content_rawsql = sqlite3_column_text(statement, 0);
            if(lemma_textarea_content_rawsql != nullptr) lemma_textarea_content = (const char*)lemma_textarea_content_rawsql;
            pos = sqlite3_column_int(statement, 1);
            if(!pos) pos = 1;
            sqlite3_finalize(statement);
        }
        else if(!existing_lemma_id) {
            sql_text = "SELECT lemma, pos, eng_trans1, eng_trans2, eng_trans3, eng_trans4, eng_trans5, eng_trans6, eng_trans7, eng_trans8, eng_trans9, eng_trans10 FROM lemmas WHERE lemma_id = ?";
            prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
            sqlite3_bind_int(statement, 1, first_lemma_id);
            run_code = sqlite3_step(statement);

            const unsigned char* lemma_rawsql = sqlite3_column_text(statement, 0);
            if(lemma_rawsql != nullptr) lemma_tag_content = (const char*)lemma_rawsql;

            pos = sqlite3_column_int(statement, 1);

            //ensure that the first non-empty meaning is returned incase some bastard fills in meaning no.10 first
            const unsigned char* lemma_textarea_content_rawsql;
            for(int i = 2; i < 12; i++) {
                lemma_textarea_content_rawsql = sqlite3_column_text(statement, i);
                if(lemma_textarea_content_rawsql != nullptr) {
                    lemma_textarea_content = (const char*)lemma_textarea_content_rawsql; //this is implicitly a std::string construction from const char*
                    lemma_meaning_no = i - 1;
                    break;
                }
            }
            sqlite3_finalize(statement);
            lemma_id = first_lemma_id;
        }
        else {
            std::string sql_text_str = "SELECT lemma, pos, eng_trans"+std::to_string(lemma_meaning_no)+" FROM lemmas WHERE lemma_id = ?";
            sql_text = sql_text_str.c_str();
            prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
            sqlite3_bind_int(statement, 1, existing_lemma_id);
            run_code = sqlite3_step(statement);

            const unsigned char* lemma_rawsql = sqlite3_column_text(statement, 0);
            if(lemma_rawsql != nullptr) lemma_tag_content = (const char*)lemma_rawsql;
            pos = sqlite3_column_int(statement, 1);

            const unsigned char* lemma_textarea_rawsql = sqlite3_column_text(statement, 2);
            if(lemma_textarea_rawsql != nullptr) lemma_textarea_content = (const char*)lemma_textarea_rawsql;

            sqlite3_finalize(statement);

            lemma_id = existing_lemma_id;
        }

        sqlite3_close(DB);


        std::ostringstream json;
        json << "{\"lemma_tag_content\":\"" << htmlspecialchars(lemma_tag_content) << "\",\"lemma_textarea_content_html\":\"" << htmlspecialchars(lemma_textarea_content) << "\",\"lemma_textarea_content\":\"" << escapeQuotes(lemma_textarea_content) << "\",\"lemma_meaning_no\":\"" << lemma_meaning_no << "\",\"lemma_id\":\"" << lemma_id << "\",\"pos\":\"" << pos << "\"}";

        int content_length = json.str().size();
        std::ostringstream post_response;
        post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << json.str();
        int length = post_response.str().size() + 1;

        sendToClient(clientSocket, post_response.str().c_str(), length);

        
        return true;
    }
    else {
        std::cout << "Database connection failed on retrieveEngword\n";
        return false;
    }
}

//this is about 1.8x slower than the PHP/MySQL version (~48ms on RaspberryPi 2) which I don't understand, possibly SQLite's per-transaction speed is slower and this is unavoidably 2 transactions
bool WebServer::recordLemma(std::string _POST[8], int clientSocket) {

    sqlite3* DB;
    sqlite3_stmt* statement;

    if(!sqlite3_open(m_DB_path, &DB)) {
        //int word_engine_id = std::stoi(_POST[0]);
                
        std::string lemma_form = URIDecode(_POST[1]);
        std::string lemma_meaning = URIDecode(_POST[2]);               
        short int lemma_meaning_no = std::stoi(_POST[3]);
        //short int lang_id = std::stoi(_POST[4]);
        //sqlite3_int64 tokno = std::stoi(_POST[5]);
        //short int pos = std::stoi(_POST[6]);
        short int submitted_lemma_meaning_no = std::stoi(_POST[7]);

        const char *sql_BEGIN = "BEGIN IMMEDIATE";
        const char *sql_COMMIT = "COMMIT";      

        //this will get skipped if we are assigning an existing lemma to a new form
        std::string sql_text_str = "INSERT OR IGNORE INTO lemmas (lemma, lang_id, pos) VALUES (\'"+lemma_form+"\', "+_POST[4]+", "+_POST[6]+")";
        const char* sql_text = sql_text_str.c_str();
        int prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        //sqlite3_bind_text(statement, 1, lemma_form.c_str(), -1, SQLITE_STATIC);
        //sqlite3_bind_int(statement, 2, lang_id);
        //sqlite3_bind_int(statement, 3, pos);
        int run_code = sqlite3_step(statement);
        sqlite3_finalize(statement);

        sqlite3_prepare_v2(DB, sql_BEGIN, -1, &statement, NULL);
        sqlite3_step(statement);
        sqlite3_finalize(statement);

        sql_text_str = "SELECT first_lemma_id FROM word_engine WHERE word_engine_id = "+_POST[0];
        //const char* sql_text = "SELECT first_lemma_id FROM word_engine WHERE word_engine_id = ?";
        sql_text = sql_text_str.c_str();
        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        //sqlite3_bind_int(statement, 1, word_engine_id);
        run_code = sqlite3_step(statement);
        int first_lemma_id = sqlite3_column_int(statement, 0);
        sqlite3_finalize(statement);

        sql_text_str = "SELECT lemma_id FROM display_text WHERE tokno = "+_POST[5];
        sql_text = sql_text_str.c_str();
        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        //sqlite3_bind_int64(statement, 1, tokno);
        run_code = sqlite3_step(statement);
        int lemma_id_current = sqlite3_column_int(statement, 0);
        sqlite3_finalize(statement);

        //get lemma_id of the lemma we just (tried to) insert (it may already have been in the table so we have to get it by its set of unique columns)
        sql_text_str = "SELECT lemma_id FROM lemmas WHERE lemma = \'"+lemma_form+"\' AND lang_id = "+_POST[4]+" AND pos = "+_POST[6];
        sql_text = sql_text_str.c_str();
        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        //sqlite3_bind_text(statement, 1, lemma_form.c_str(), -1, SQLITE_STATIC);
        //sqlite3_bind_int(statement, 2, lang_id);
        //sqlite3_bind_int(statement, 3, pos);
        run_code = sqlite3_step(statement);
        int target_lemma_id = sqlite3_column_int(statement, 0);
        sqlite3_finalize(statement);

        sql_text_str = "UPDATE lemmas SET eng_trans"+_POST[3]+" = \'"+lemma_meaning+"\' WHERE lemma_id = ?";
        sql_text = sql_text_str.c_str();
        std::cout << sql_text_str << std::endl;
        std::cout << "target_lemma_id: " << target_lemma_id << std::endl;
        sql_text = sql_text_str.c_str();
        prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
        //sqlite3_bind_text(statement, 1, lemma_meaning.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(statement, 1, target_lemma_id);
        run_code = sqlite3_step(statement);
        sqlite3_finalize(statement);

        if(!lemma_id_current) {

            if(submitted_lemma_meaning_no == lemma_meaning_no) {
                sql_text_str = "UPDATE display_text SET lemma_id = ?, lemma_meaning_no = "+_POST[7]+" WHERE tokno = "+_POST[5];
                sql_text = sql_text_str.c_str();
                prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
                sqlite3_bind_int(statement, 1, target_lemma_id);
                //sqlite3_bind_int(statement, 2, submitted_lemma_meaning_no);
                //sqlite3_bind_int64(statement, 3, tokno);
                run_code = sqlite3_step(statement);
                sqlite3_finalize(statement);
            }
        }
        else {
            if(lemma_id_current != target_lemma_id) {
                if(lemma_meaning_no == submitted_lemma_meaning_no) {
                    sql_text_str = "UPDATE display_text SET lemma_id = ?, lemma_meaning_no = "+_POST[7]+" WHERE tokno = "+_POST[5];
                    sql_text = sql_text_str.c_str();
                    prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
                    sqlite3_bind_int(statement, 1, target_lemma_id);
                    //sqlite3_bind_int(statement, 2, submitted_lemma_meaning_no);
                    //sqlite3_bind_int64(statement, 3, tokno);
                    run_code = sqlite3_step(statement);
                    sqlite3_finalize(statement);
                }       
            }
            //if the lemma we are submitting is the same as that already bound to this display_word then we leave the lemma_id unchanged as just re-set the lemma_meaning_no
            else {
                if(lemma_meaning_no = submitted_lemma_meaning_no) {
                    sql_text_str = "UPDATE display_text SET lemma_meaning_no = "+_POST[7]+" WHERE tokno = "+_POST[5];
                    sql_text = sql_text_str.c_str();
                    prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
                    //sqlite3_bind_int(statement, 1, submitted_lemma_meaning_no);
                    //sqlite3_bind_int64(statement, 2, tokno);
                    run_code = sqlite3_step(statement);
                }
            }
        }

        if(!first_lemma_id) {
            sql_text_str = "UPDATE word_engine SET first_lemma_id = ? WHERE word_engine_id = "+_POST[0];
            sql_text = sql_text_str.c_str();
            prep_code = sqlite3_prepare_v2(DB, sql_text, -1, &statement, NULL);
            sqlite3_bind_int(statement, 1, target_lemma_id);
            //sqlite3_bind_int(statement, 2, word_engine_id);
            run_code = sqlite3_step(statement);
            sqlite3_finalize(statement); 
        }


        sqlite3_prepare_v2(DB, sql_COMMIT, -1, &statement, NULL);
        sqlite3_step(statement);
        sqlite3_finalize(statement);



        sqlite3_close(DB);

        std::string POST_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
        int size = POST_response.size() + 1;

        sendToClient(clientSocket, POST_response.c_str(), size);
        return true;
    }
    else {
        std::cout << "Database connection failed on recordLemma\n";
        return false;
    }

}

bool WebServer::pullInLemma(std::string _POST[4], int clientSocket) {
    
    sqlite3* DB;
    sqlite3_stmt* statement;

    if(!sqlite3_open(m_DB_path, &DB)) {

        std::string sql_text_str = "";
        short int pos = std::stoi(_POST[2]);
        if(pos == 0) {
            sql_text_str = "SELECT eng_trans"+_POST[1]+", lemma_id, pos FROM lemmas WHERE lemma = \'"+URIDecode(_POST[0])+"\' AND lang_id = "+_POST[3];
        }
        else {
            sql_text_str = "SELECT eng_trans"+_POST[1]+", lemma_id FROM lemmas WHERE lemma = \'"+URIDecode(_POST[0])+"\' AND pos = "+_POST[2]+" AND lang_id = "+_POST[3];
        }
        
        std::cout << sql_text_str << std::endl;

        sqlite3_prepare_v2(DB, sql_text_str.c_str(), -1, &statement, NULL);
        sqlite3_step(statement);
        int lemma_id = sqlite3_column_int(statement, 1);
        std::string lemma_textarea_content;
        if(lemma_id) {
            lemma_textarea_content = sqlite3_column_text(statement, 0) == nullptr ? "" : (const char*)sqlite3_column_text(statement, 0);
        }
        if(pos == 0) {
            pos = sqlite3_column_int(statement, 2);
        }
        
        sqlite3_finalize(statement);

        sqlite3_close(DB);

        std::ostringstream json;

        if(!lemma_id) {
            json << "{\"lemma_textarea_content\":null,\"lemma_id\":null,\"pos\":null}";
        }
        else {
            json << "{\"lemma_textarea_content\":\"" << escapeQuotes(lemma_textarea_content) << "\",\"lemma_id\":\"" << lemma_id << "\",\"pos\":\"" << pos << "\"}";
        }

        int content_length = json.str().size();
        std::ostringstream post_response;
        post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << json.str();
        int length = post_response.str().size() + 1;

        sendToClient(clientSocket, post_response.str().c_str(), length);

       
        
        return true;
    }
    else {
        std::cout << "Database connection failed on pullInLemma()" << std::endl;
        return false;
    }

}

bool WebServer::retrieveMeanings(std::string _POST[2], int clientSocket) {
    
    sqlite3* DB;
    sqlite3_stmt* statement;

    if(!sqlite3_open(m_DB_path, &DB)) {
        int prep_code, run_code;
        std::string sql_text_str = "SELECT eng_trans"+_POST[1]+" FROM lemmas WHERE lemma_id = "+_POST[0];
        prep_code = sqlite3_prepare_v2(DB, sql_text_str.c_str(), -1, &statement, NULL);
        run_code = sqlite3_step(statement);

        std::string lemma_textarea_content = "";
        const unsigned char* lemma_textarea_content_rawsql = sqlite3_column_text(statement, 0);
        if(lemma_textarea_content_rawsql != nullptr) lemma_textarea_content = (const char*)lemma_textarea_content_rawsql;

        sqlite3_finalize(statement);

        sqlite3_close(DB);

        int content_length = lemma_textarea_content.size();

        std::ostringstream post_response;
        post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << lemma_textarea_content;

        int length = post_response.str().size() + 1;

        sendToClient(clientSocket, post_response.str().c_str(), length);

        
        return true;
    }
    else {
        std::cout << "Database connection failed on retrieveMeanings()" << std::endl;
        return false;
    }
}

bool WebServer::deleteLemma(std::string _POST[3], int clientSocket) {

    sqlite3* DB;

    if(!sqlite3_open(m_DB_path, &DB)) {
        const char *sql_BEGIN = "BEGIN IMMEDIATE";
        const char *sql_COMMIT = "COMMIT";
        sqlite3_stmt* statement;

        bool lemma_still_set = true;

        std::string sql_text_str = "SELECT lemma_id FROM display_text WHERE word_engine_id = "+_POST[1]+" AND lemma_id IS NOT NULL AND lemma_id != "+_POST[0];

        sqlite3_prepare_v2(DB, sql_text_str.c_str(), -1, &statement, NULL);
        sqlite3_step(statement);
        int leftover_lemma_id = sqlite3_column_int(statement, 0);
        sqlite3_finalize(statement);
        //could be less code by just if/elseing the sql_text_str but this way is clearer and not less efficient
        if(!leftover_lemma_id) {
            std::cout << "no leftover_lemma_id" << std::endl;

            sqlite3_prepare_v2(DB, sql_BEGIN, -1, &statement, NULL);
            sqlite3_step(statement);
            sqlite3_finalize(statement);

            sql_text_str = "UPDATE word_engine SET first_lemma_id = NULL WHERE word_engine_id = "+_POST[1];
            sqlite3_prepare_v2(DB, sql_text_str.c_str(), -1, &statement, NULL);
            sqlite3_step(statement);
            sqlite3_finalize(statement);

            sql_text_str = "UPDATE display_text SET lemma_meaning_no = NULL, lemma_id = NULL WHERE word_engine_id = "+_POST[1];
            sqlite3_prepare_v2(DB, sql_text_str.c_str(), -1, &statement, NULL);
            sqlite3_step(statement);
            sqlite3_finalize(statement);

            sqlite3_prepare_v2(DB, sql_COMMIT, -1, &statement, NULL);
            sqlite3_step(statement);
            sqlite3_finalize(statement);

            sql_text_str = "SELECT word_engine_id FROM word_engine WHERE first_lemma_id = "+_POST[0];
            sqlite3_prepare_v2(DB, sql_text_str.c_str(), -1, &statement, NULL);
            sqlite3_step(statement);
            int other_lemma_form = sqlite3_column_int(statement, 0);
            sqlite3_finalize(statement);

            if(!other_lemma_form) {
                std::cout << "no other_lemma_form" << std::endl;
                sql_text_str = "DELETE FROM lemmas WHERE lemma_id = "+_POST[0];
                sqlite3_prepare_v2(DB, sql_text_str.c_str(), -1, &statement, NULL);
                sqlite3_step(statement);
                sqlite3_finalize(statement);
            }
            lemma_still_set = false;
        }
        else {
            sqlite3_prepare_v2(DB, sql_BEGIN, -1, &statement, NULL);
            sqlite3_step(statement);
            sqlite3_finalize(statement);

            sql_text_str = "UPDATE word_engine SET first_lemma_id = "+std::to_string(leftover_lemma_id)+" WHERE word_engine_id = "+_POST[1];
            sqlite3_prepare_v2(DB, sql_text_str.c_str(), -1, &statement, NULL);
            sqlite3_step(statement);
            sqlite3_finalize(statement);

            sql_text_str = "UPDATE display_text SET lemma_meaning_no = NULL, lemma_id = NULL WHERE lemma_id = "+_POST[0];
            sqlite3_prepare_v2(DB, sql_text_str.c_str(), -1, &statement, NULL);
            sqlite3_step(statement);
            sqlite3_finalize(statement);

            sqlite3_prepare_v2(DB, sql_COMMIT, -1, &statement, NULL);
            sqlite3_step(statement);
            sqlite3_finalize(statement);

            sql_text_str = "SELECT word_engine_id FROM word_engine WHERE first_lemma_id = "+_POST[0];
            sqlite3_prepare_v2(DB, sql_text_str.c_str(), -1, &statement, NULL);
            sqlite3_step(statement);
            int other_lemma_form = sqlite3_column_int(statement, 0);
            sqlite3_finalize(statement);

            if(!other_lemma_form) {
                std::cout << "no other_lemma_form" << std::endl;
                sql_text_str = "DELETE FROM lemmas WHERE lemma_id = "+_POST[0];
                sqlite3_prepare_v2(DB, sql_text_str.c_str(), -1, &statement, NULL);
                sqlite3_step(statement);
                sqlite3_finalize(statement);
            }
            lemma_still_set = true;
        }
        sqlite3_close(DB);
        
        std::ostringstream POST_response;
        POST_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 1\r\n\r\n" << lemma_still_set;
        int size = POST_response.str().size() + 1;


        sendToClient(clientSocket, POST_response.str().c_str(), 65);
        
        return true;
    }
    else {
        std::cout << "Database connection failed on deleteLemma()" << std::endl;
        return false;
    }

}