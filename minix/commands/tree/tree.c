#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

typedef struct Node
{
    char* name;
    int depth;
    struct Node* first_son;
    struct Node* last_son;
    struct Node* next_sibling;
} Node;



Node* CreateNode(const char* name, int depth);
void InsertChild(Node* father, Node* child);
void PrintTree(Node* node);
int GetFileInfo(const char* path, struct stat* info);
void ScanDirectory(Node* node, const char* path);

int main(int argc, char* argv[])
{
    char path[PATH_MAX];
    Node* root_node;

    if (argc < 2)
        strcpy(path, ".");
    else
    {
        strcpy(path, argv[1]);
        struct stat info;
        if (lstat(path, &info) == -1)
        {
            fprintf(stderr, "tree: No se puede acceder a '%s': %s\n",
                    path, strerror(errno));
            return 1;
        }
    }

    const char* name_root;
    if (strcmp(path, ".") == 0) {
        name_root = ".";
    } else if (strcmp(path, "/") == 0) {
        name_root = "/";
    } else {
        name_root = strrchr(path, '/');
        name_root = (name_root == NULL) ? path : name_root + 1;
    }

    root_node = CreateNode(name_root, 0);

    if (root_node == NULL)
    {
        fprintf(stderr, "Error: No se pudo crear el nodo raíz\n");
        return 1;
    }

    struct stat info;
    bool isDir = false;
    bool isLink = true;
    if (GetFileInfo(path, &info) == 0)
    {
        isDir = S_ISDIR(info.st_mode);
        isLink = S_ISLNK(info.st_mode);
    }
    if (isDir && !isLink)
    {
        ScanDirectory(root_node, path);
    }
    PrintTree(root_node);
    free(root_node);
    return 0;
}

Node* CreateNode(const char* name, int depth)
{
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL)
    {
        printf("Error: No se pudo asignar memoria\n");
        return NULL;
    }
    new_node->name = (char*)malloc(strlen(name) + 1);
    if (new_node->name == NULL)
    {
        free(new_node);
        return NULL;
    }
    strcpy(new_node->name, name);
    new_node->depth = depth;
    new_node->first_son = NULL;
    new_node->last_son = NULL;
    new_node->next_sibling = NULL;
    return new_node;
}


void InsertChild(Node* father, Node* child)
{
    if (father == NULL)
    {
        printf("Error: Padre no válido\n");
        return;
    }
    if (father->first_son == NULL)
    {
        father->first_son = child;
        father->last_son = child;
    }
    else
    {
        father->last_son->next_sibling = child;
        father->last_son = child;
    }
}

void PrintTree(Node* node)
{
    if (node == NULL) return;

    for (int i = 0; i < node->depth; i++)
    {
        printf("  ");
    }

    printf("├─ %s\n", node->name);

    Node* child_node = node->first_son;
    while (child_node != NULL)
    {
        PrintTree(child_node);
        child_node = child_node->next_sibling;
    }
}

int GetFileInfo(const char* path, struct stat* info)
{
    if (lstat(path, info) == -1)
    {
        return -1;
    }
    return 0;
}

void ScanDirectory(Node* node, const char* path)
{
    DIR* dir;
    struct dirent* entry;
    struct stat info;

    if (node == NULL) return;

    dir = opendir(path);
    if (dir == NULL)
    {
        fprintf(stderr, "Error al abrir directorio %s: %s\n", path, strerror(errno));
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[PATH_MAX];
        if (strcmp(path, "/") == 0)
        {
            snprintf(full_path, sizeof(full_path), "/%s", entry->d_name);
        }
        else if (strcmp(path, ".") == 0)
        {
            snprintf(full_path, sizeof(full_path), "%s", entry->d_name);
        }
        else
        {
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        }

        if (GetFileInfo(full_path, &info) == -1)
        {
            fprintf(stderr, "Error al obtener info de %s: %s\n", full_path, strerror(errno));
            continue;
        }

        if (S_ISLNK(info.st_mode)) continue;

        Node* child_node = CreateNode(entry->d_name, node->depth + 1);
        if (child_node == NULL) continue;

        bool IsDirectory = S_ISDIR(info.st_mode);
        if (IsDirectory)
        {
            ScanDirectory(child_node, full_path);
        }
        InsertChild(node, child_node);
    }
    closedir(dir);
}
