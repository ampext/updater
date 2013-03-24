import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.MessageDigest;
import java.util.Formatter;
import java.util.Stack;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.Options;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class PackageMaker
{
	static public class UpdateInfo
	{
		public String app_name = "";
		public String app_version = "";
		public String process_name = "";
		
		public String pkg_filename = "";
		public String pkg_url = "";
		public String pkg_hash = "";
		public long pkg_size = 0;
	}
	
    public static void main(String[] args)
    {		
		try
		{
			Options options = new Options();
			{
				options.addOption("a", "app_version", true, "application version");
				options.addOption("n", "app_name", true, "application name");
				options.addOption("p", "process_name", true, "application process name");
				options.addOption("o", "output", true, "output package name");
				options.addOption("u", "url", true, "package url");
			}
			
			CommandLineParser parser = new GnuParser();
			CommandLine cmd = parser.parse(options, args);
			String[] agruments = cmd.getArgs();
			
			if(agruments.length == 0) throw new Exception("application path is not specified");
			if(!cmd.hasOption("output")) throw new Exception("output package name is not specified");
			if(!cmd.hasOption("app_name")) throw new Exception("application name is not specified");
			if(!cmd.hasOption("app_version")) throw new Exception("application version is not specified");
			if(!cmd.hasOption("process_name")) throw new Exception("application process name is not specified");
			
			UpdateInfo info = new UpdateInfo();
			
			info.pkg_filename = cmd.getOptionValue("output");
			info.pkg_url = cmd.hasOption("url") ? cmd.getOptionValue("url") : "";
			info.app_name = cmd.getOptionValue("app_name");
			info.app_version = cmd.getOptionValue("app_version");
			info.process_name = cmd.getOptionValue("process_name");
				
			makePackage(info, agruments[0]);
		}
		catch(Exception e)
		{
			System.err.println(e.getMessage());
		}
    }
	
	public static boolean makePackage(UpdateInfo info, String src_dir)
	{
		try
		{
			System.out.println("Making archive...");
			if(!makeArchive(info.pkg_filename, src_dir)) return false;

			System.out.println("Calculating hash (SHA1)...");

			File file = new File(info.pkg_filename);

			if(info.pkg_url.isEmpty()) info.pkg_url = file.getName();
			info.pkg_size = file.length();
			info.pkg_hash = getFileSHA1(file);
			if(info.pkg_hash.isEmpty()) System.exit(1);

			System.out.println("Generating update info...");
			if(!writeUpdateInfo(file.getParentFile().getAbsolutePath() + File.separator + "update_info.xml", info)) System.exit(1);
		}
		catch(Exception e)
		{
			System.err.println(e.getMessage());
			return false;
		}
		
		return true;
	}

	public static boolean makeArchive(String dst_path, String src_path) throws FileNotFoundException, IOException
	{
		FileOutputStream fileStream = new FileOutputStream(dst_path);
		boolean result = true;
		
		try(ZipOutputStream zipStream = new ZipOutputStream(fileStream))
		{
			Stack<File> files = new Stack<>();

			File root = new File(src_path);
			Path root_path;

			if(!root.exists())
			{
				System.out.println("Nothing to package");
				return false;
			}
			else 
			{
				if(root.isFile())
				{
					files.push(root);
					root_path = Paths.get(root.getParent()).toAbsolutePath();
				}
				else if(root.isDirectory())
				{
					listFiles(root, files);
					root_path = Paths.get(src_path).toAbsolutePath();
				}
				else return false;
			}

			while(!files.empty())
			{
				File file = files.pop();
				Path rel_path = root_path.relativize(Paths.get(file.getAbsolutePath()));
		
				if(file.isFile())				
				{
					System.out.print("Compressing " + rel_path + "... ");
					boolean res = writeFileToZip(zipStream, file, rel_path.toString());
					result = result && res;
					
					System.out.println(res ? "OK" : "FAIL");
				}
				else if(file.isDirectory())
				{
					listFiles(file, files);
					
					String dir_path = rel_path.toString();
					if(!dir_path.endsWith("/")) dir_path += "/";
					
					ZipEntry zipEntry = new ZipEntry(dir_path);
					zipStream.putNextEntry(zipEntry);
					zipStream.closeEntry();
					
					System.out.println("Creating path " + rel_path + "... OK");
				}
			}
		}
		
		return result;
	}
	
	public static void listFiles(File dir, Stack<File> files) throws IOException
	{
		if(!dir.exists()) throw new IOException("Directory " + dir.getAbsolutePath() + " does not exists");
		if(!dir.isDirectory()) throw new IllegalArgumentException();
		
		File[] dir_files = dir.listFiles();
		
		for(File file : dir_files)
			files.push(file);
	}
	
	public static boolean writeFileToZip(ZipOutputStream zipStream, File file, String altName)
	{
		try
		{
			ZipEntry zipEntry = new ZipEntry(altName);
			zipStream.putNextEntry(zipEntry);
			
			try(FileInputStream fileStream = new FileInputStream(file))
			{
				byte[] buf = new byte[10000];
				int readed = -1;
				
				while((readed = fileStream.read(buf)) != -1)
					zipStream.write(buf, 0, readed);
			}
			
			zipStream.closeEntry();
		}
		catch(Exception e)
		{
			System.err.println(e.getMessage());
			return false;
		}

		return true;
	}
	
	public static boolean writeUpdateInfo(String filename, UpdateInfo info)
	{
		try
		{
			DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
			DocumentBuilder builder = factory.newDocumentBuilder();
			Document doc = builder.newDocument();
			
			Element root = doc.createElement("update");
			root.setAttribute("format_version", "1");
			doc.appendChild(root);
			
			Element app_node = doc.createElement("app");
			app_node.setAttribute("name", info.app_name);
			app_node.setAttribute("process_name", info.process_name);
			app_node.setAttribute("version", info.app_version);
			root.appendChild(app_node);

			Element elements_node = doc.createElement("elements");
			app_node.appendChild(elements_node);

			Element element_node = doc.createElement("element");
			element_node.setAttribute("type", "update_bundle");
			elements_node.appendChild(element_node);

			Element file_node = doc.createElement("file");
			file_node.setAttribute("src", info.pkg_url);
			if(!info.pkg_hash.isEmpty()) file_node.setAttribute("hash", info.pkg_hash);
			file_node.setAttribute("size", info.pkg_size + "");
			element_node.appendChild(file_node);
			
			TransformerFactory transformerFactory = TransformerFactory.newInstance();
			Transformer transformer = transformerFactory.newTransformer();
			DOMSource source = new DOMSource(doc);

			StreamResult result = new StreamResult(new File(filename));
			transformer.setOutputProperty(OutputKeys.INDENT, "yes");
			transformer.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "4");
			transformer.transform(source, result);
		}
		catch(Exception e)
		{
			System.err.println(e.getMessage());
			return false;
		}
		
		return true;
	}
	
	public static String getFileSHA1(File file)
	{
		Formatter formatter = new Formatter();
		
		try
		{
			MessageDigest md = MessageDigest.getInstance("SHA-1"); 

			try(FileInputStream fileStream = new FileInputStream(file))
			{
				byte[] buf = new byte[10000];
				int readed = -1;

				while((readed = fileStream.read(buf)) != -1)
				{
					md.update(buf, 0, readed);
				}

				byte[] hash = md.digest();

				for(byte b : hash)
					formatter.format("%02x", b);
			}
		}
		catch(Exception e)
		{
			System.err.println(e.getMessage());
			return "";
		}
		
		return formatter.toString();
	}
}