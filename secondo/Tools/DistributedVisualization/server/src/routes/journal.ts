import { JournalController } from "../controllers/Journal";
import { RouteInterface } from "./RouteInterface";

const JOURNAL_BASE: string = "journal";
export function journalRoutes(): RouteInterface[] {
  return [
    {
      method: 'GET',
      path: `/${JOURNAL_BASE}`,
      validators: [],
      authenticators: [],
      handler: JournalController.getJournal,
    }
  ];
} 