#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h> /* for realloc */
#include <string.h> /* for memcpy */
#include <ctype.h>

// Global function
char *CURL_word_definitions(const char *word);

// Local function
static void filter_word(const char *word, char *fWord);
static size_t cb(char *data, size_t size, size_t nmemb, void *clientp);
static char *parse_dictionary_json(const char *data, char *filteredWord);
static char *get_dictionary_line(const char *title , const char *titStr, char resetF);

struct memory {
  char *response;
  size_t size;
};

static size_t cb(char *data, size_t size, size_t nmemb, void *clientp)
{
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)clientp;

  char *ptr = realloc(mem->response, mem->size + realsize + 1);
  if(!ptr)
  {
    return 0;  /* out of memory */
  }
  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;

  return realsize;
}

 char* print_string_array(cJSON *array)
{
    char buf[256];
    char *arrayString = NULL;
    int arraySize = 0;
    cJSON *item;
    cJSON *text;
    cJSON *jsonObj;

    if (cJSON_IsArray(array) && cJSON_GetArraySize(array) > 0)
    {
        for (int i = 0 ; i < cJSON_GetArraySize(array) ; i++)
        {
            item = cJSON_GetArrayItem(array, i);        // for Sysnonyms and Antonyms
            text = cJSON_GetObjectItem(item, "text");   // for Phonetics

            jsonObj = text ? text : item;

            if(cJSON_IsString(jsonObj) && jsonObj->valuestring)
            {
                sprintf(buf, "%s%s", jsonObj->valuestring, (i < cJSON_GetArraySize(array) - 1) ? ", " : ".");

                arraySize += strlen(buf) + 1;

                if(!arrayString)
                  arrayString = calloc(arraySize + 1, sizeof(char));
                else
                  arrayString  =  realloc(arrayString, arraySize + 1);

                strcat(arrayString, buf);
            }

         }

    }

    return arrayString;
}

char *parse_dictionary_json(const char *data,  char *filteredWord)
{
  char *dicLines = NULL, *errorMsg,
       *synonymsStr, *antonymsStr, *phoneticsStr;

  cJSON *root, *entry, *word,
        *phonetics, *phonetic, *text,
        *meanings, *meaning, *partOfSpeech,
        *definitions, *def, *definition,
        *example, *synonyms, *antonyms;

    root = cJSON_Parse(data);

    if(!root || !cJSON_IsArray(root))
    {
        char *errorMsg;

        if(strlen(filteredWord) > 50)
        {
            errorMsg = malloc(strlen("Chosen word is too long.") + 1);
            sprintf(errorMsg, "Chosen word is too long.");
        }
        else
        {
          errorMsg = malloc(strlen("Sorry, Couldn't find definitions for \"\" .") + strlen(filteredWord) + 1);
          sprintf(errorMsg, "Sorry, Couldn't find definitions for \"%s\".", filteredWord);
        }

        dicLines = get_dictionary_line("ERROR: ", errorMsg, 1);
        cJSON_Delete(root);
        free(errorMsg);
        return dicLines;
    }

    for(int i = 0 ; i < cJSON_GetArraySize(root) ; ++i)
    {
        if(i)
          get_dictionary_line(" ", "", 0);   // new line

        entry = cJSON_GetArrayItem(root, i);

        word = cJSON_GetObjectItem(entry, "word");
        if(word && cJSON_IsString(word))
          get_dictionary_line("Word: ", word->valuestring, 0);

        phonetics = cJSON_GetObjectItem(entry, "phonetics");

        if(phoneticsStr = print_string_array(phonetics))
        {
            get_dictionary_line("Phonetics: ", phoneticsStr, 0);
            free(phoneticsStr);
        }


        meanings = cJSON_GetObjectItem(entry, "meanings");
        if(meanings && cJSON_IsArray(meanings))
        {
            get_dictionary_line(" ", "", 0);   // new line
            for(int k = 0 ; k < cJSON_GetArraySize(meanings) ; ++k)
            {
                meaning = cJSON_GetArrayItem(meanings, k);
                partOfSpeech = cJSON_GetObjectItem(meaning, "partOfSpeech");

                if(partOfSpeech)
                {
                  get_dictionary_line("Part of Speech: ", partOfSpeech->valuestring, 0);
                  get_dictionary_line(" ", "", 0);   // new line
                }

                // Definitions
                definitions = cJSON_GetObjectItem(meaning, "definitions");
                if(definitions && cJSON_IsArray(definitions))
                {
                    for(int d = 0 ; d < cJSON_GetArraySize(definitions) ; ++d)
                    {
                        def = cJSON_GetArrayItem(definitions, d);
                        definition = cJSON_GetObjectItem(def, "definition");
                        example = cJSON_GetObjectItem(def, "example");
                        synonyms = cJSON_GetObjectItem(def, "synonyms");
                        antonyms = cJSON_GetObjectItem(def, "antonyms");

                        if(definition)
                            get_dictionary_line("Definition: ", definition->valuestring, 0);


                        if(example)
                            get_dictionary_line("Example: ", example->valuestring, 0);

                        if(synonyms)
                            if(synonymsStr = print_string_array(synonyms))
                            {
                                get_dictionary_line("Synonyms: ", synonymsStr, 0);
                                free(synonymsStr);
                            }

                        if(antonyms)
                            if(antonymsStr = print_string_array(antonyms))
                            {
                                get_dictionary_line("Antonyms: ", antonymsStr, 0);
                                free(antonymsStr);
                            }

                            get_dictionary_line(" ", "", 0);   // new line
                    }
                }

                synonyms = cJSON_GetObjectItem(meaning, "synonyms");
                antonyms = cJSON_GetObjectItem(meaning, "antonyms");
                if(synonyms || antonyms)
                {
                    if(synonymsStr = print_string_array(synonyms))
                    {
                      get_dictionary_line("Synonyms: ", synonymsStr, 0);
                      free(synonymsStr);
                    }
                    if(antonymsStr = print_string_array(antonyms))
                    {
                      get_dictionary_line("Antonyms: ", antonymsStr, 0);
                      free(antonymsStr);
                    }

                    if(synonymsStr || antonymsStr)
                      get_dictionary_line(" ", "", 0);   // new line

                }

            }
        }


    }

    dicLines = get_dictionary_line("", "", 1);
    cJSON_Delete(root);

    return dicLines;
}


char *CURL_word_definitions(const char *word)
{

  struct memory chunk = {0};
    CURLcode res;

    int wordLen = strlen(word);
    char url[] = "https://api.dictionaryapi.dev/api/v2/entries/en/";
    int urlLen = strlen(url);
    char *fullURL,
         *fWord;


    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();

    char *dictionaryOutput = NULL;

  if(curl) {

    fWord = malloc(wordLen + 1);

    filter_word(word, fWord);

    fullURL = calloc(strlen(fWord) + strlen(url) + 1, 1);
    strcpy(fullURL, url);
    strncat(fullURL, fWord, urlLen + wordLen);

    curl_easy_setopt(curl, CURLOPT_URL, fullURL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    res = curl_easy_perform(curl);

     if (res == CURLE_OK)
            dictionaryOutput = parse_dictionary_json(chunk.response, fWord);
      else
      {
         get_dictionary_line("ERROR: ", "Network connection failure.", 0);
         dictionaryOutput = get_dictionary_line("", "check internet connection.", 1);
      }

    free(chunk.response);

    curl_easy_cleanup(curl);
  }

   free(fWord);
   free(fullURL);

   curl_global_cleanup();
   return dictionaryOutput;
}

static char *get_dictionary_line(const char *title , const char *titStr, char resetF)
{
    static char *dicLines = NULL;
    static unsigned int totSize = 0;    // total Size
    char *ptr;

    if(!dicLines)
    {
        totSize = strlen(title) + strlen(titStr) + 2;     // '\n' + '\0' = 2
        dicLines = malloc(totSize);
        if(!dicLines)
        {
          fprintf(stderr, "Couldn't allocate memory.\n");
          exit(-1);
        }
        sprintf(dicLines, "%s%s\n", title, titStr);
    }
    else
    {
        totSize += strlen(title) + strlen(titStr) + 2; // '\n' + '\0' = 2
        dicLines = realloc(dicLines, totSize);
        if(!dicLines)
        {
            fprintf(stderr, "Couldn't allocate memory.\n");
            exit(-1);
        }

        strcat(dicLines, title);
        strcat(dicLines, titStr);
        strcat(dicLines, "\n");
    }

    ptr = dicLines;

    if(resetF)
    {
      dicLines = NULL;
      totSize = 0;
    }

    return ptr;
}

static void filter_word(const char *word, char *fWord)
{

  int j = 0;
  for(int i = 0; word[i] ; ++i)
  {
    if(isalpha(word[i]))
      fWord[j++] = word[i];

    else if( (isspace(word[i]) || word[i] == '-') && (isalpha(word[i-1]) && isalpha(word[i+1])) )
      fWord[j++] = '-';
  }
  fWord[j] = '\0';
}
