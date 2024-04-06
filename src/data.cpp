#include "data.h"

void listDir(fs::FS &fs, const char *dirname)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname, "r");
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char *path)
{
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path))
    {
        Serial.println("Dir created");
    }
    else
    {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char *path)
{
    Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path))
    {
        Serial.println("Dir removed");
    }
    else
    {
        Serial.println("rmdir failed");
    }
}

String readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path, "r");
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return "";
    }

    String output = "";
    Serial.println("- read from file:");
    while (file.available())
    {
        output += file.readStringUntil('\n');
        output += '\n';
    }
    file.close();
    return output;
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, "w");
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, "a");
    if (!file)
    {
        Serial.println("- failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- message appended");
    }
    else
    {
        Serial.println("- append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2)
{
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2))
    {
        Serial.println("- file renamed");
    }
    else
    {
        Serial.println("- rename failed");
    }
}

void deleteFile(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\r\n", path);
    if (fs.remove(path))
    {
        Serial.println("- file deleted");
    }
    else
    {
        Serial.println("- delete failed");
    }
}

void checkAndCleanFileSystem(fs::FS &fs)
{
    FSInfo fs_info;
    fs.info(fs_info);
    float freeSpacePercentage = ((float)(fs_info.totalBytes - fs_info.usedBytes) / fs_info.totalBytes) * 100;

    Serial.print("Free space percentage: ");
    Serial.println(freeSpacePercentage);

    if (freeSpacePercentage < 10)
    {
        Serial.println("Free space is less than 10%, cleaning up...");

        Dir dir = fs.openDir("/");
        std::vector<std::pair<String, time_t>> files;

        while (dir.next())
        {
            if (!dir.fileName().isEmpty() && Communication::get_instance()->is_older_than_five_days(dir.fileName()))
            {
                fs.remove(dir.fileName());
            }
        }
    }
    else
    {
        Serial.println("Sufficient free space available.");
    }
}

int getFileSize(fs::FS &fs, const char *path)
{
    File file = fs.open(path, "r");
    if (!file)
    {
        Serial.println("- failed to open file for appending");
        return -1;
    }
    size_t f_size = file.size();
    file.close();
    return f_size;
}